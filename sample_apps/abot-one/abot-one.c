#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <limits.h>
#include <wiringSerial.h>
#include <errno.h>

#include "aws_iot_log.h"
#include "aws_iot_version.h"
#include "aws_iot_mqtt_interface.h"
#include "aws_iot_shadow_interface.h"
#include "aws_iot_config.h"
#include <wiringPi.h>

static uint8_t forward = 1;
static uint8_t backward = 2;
static uint8_t left = 3;
static uint8_t right = 4;
static uint8_t standup = 6;
static uint8_t stop = 6;
static uint8_t sitdown = 7;

char certDirectory[PATH_MAX + 1] = "../../certs";
char HostAddress[255] = AWS_IOT_MQTT_HOST;
uint32_t port = AWS_IOT_MQTT_PORT;
bool messageArrivedOnDelta = false;
/*
char botStatusValue[8];
char *pbotStatusValue = botStatusValue;
*/
char botStatusDelta[SHADOW_MAX_SIZE_OF_RX_BUFFER];

uint8_t botStatusValue = 7;

// Helper functions
void parseInputArgsForConnectParams(int argc, char** argv);

void move(int fd);

// Shadow Callback for receiving the delta
void botStatusCallback(const char *pJsonValueBuffer, uint32_t valueLength, jsonStruct_t *pJsonStruct_t);
//void lightCallback(const char *pJsonValueBuffer, uint32_t valueLength, jsonStruct_t *pJsonStruct_t);
//void musicCallback(const char *pJsonValueBuffer, uint32_t valueLength, jsonStruct_t *pJsonStruct_t);

void UpdateStatusCallback(const char *pThingName, ShadowActions_t action, Shadow_Ack_Status_t status,
		const char *pReceivedJsonDocument, void *pContextData);


int main(int argc, char** argv) {
	IoT_Error_t rc = NONE_ERROR;
	int32_t i = 0;

	int fd;
	char rootCA[PATH_MAX + 1];
	char clientCRT[PATH_MAX + 1];
	char clientKey[PATH_MAX + 1];
	char CurrentWD[PATH_MAX + 1];
	char cafileName[] = AWS_IOT_ROOT_CA_FILENAME;
	char clientCRTName[] = AWS_IOT_CERTIFICATE_FILENAME;
	char clientKeyName[] = AWS_IOT_PRIVATE_KEY_FILENAME;

	INFO("\nAWS IoT SDK Version %d.%d.%d-%s\n", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, VERSION_TAG);

	getcwd(CurrentWD, sizeof(CurrentWD));
	sprintf(rootCA, "%s/%s/%s", CurrentWD, certDirectory, cafileName);
	sprintf(clientCRT, "%s/%s/%s", CurrentWD, certDirectory, clientCRTName);
	sprintf(clientKey, "%s/%s/%s", CurrentWD, certDirectory, clientKeyName);

	DEBUG("rootCA %s", rootCA);DEBUG("clientCRT %s", clientCRT);DEBUG("clientKey %s", clientKey);

	parseInputArgsForConnectParams(argc, argv);

	// initialize the mqtt client
	MQTTClient_t mqttClient;
	aws_iot_mqtt_init(&mqttClient);

	ShadowParameters_t sp = ShadowParametersDefault;
	sp.pMyThingName = AWS_IOT_MY_THING_NAME;
	sp.pMqttClientId = AWS_IOT_MQTT_CLIENT_ID;
	sp.pHost = AWS_IOT_MQTT_HOST;
	sp.port = AWS_IOT_MQTT_PORT;
	sp.pClientCRT = clientCRT;
	sp.pClientKey = clientKey;
	sp.pRootCA = rootCA;



	if ((fd = serialOpen("/dev/ttyUSB0", 115200)) < 0)
	{
		fprintf(stderr, "Unable to open serial device: %s\n", strerror(errno));
		return 1;
	}

	if(wiringPiSetup() == -1)
	{
		fprintf(stdout, "Unable to start wiringPi: %s\n", strerror(errno));
		return 1;
	}

	INFO("Shadow Init");
	rc = aws_iot_shadow_init(&mqttClient);
	if (NONE_ERROR != rc) {
		ERROR("Shadow Connection Error");
		return rc;
	}

	INFO("Shadow Connect");
	rc = aws_iot_shadow_connect(&mqttClient, &sp);
	if (NONE_ERROR != rc) {
		ERROR("Shadow Connection Error");
		return rc;
	}

	jsonStruct_t botStatusObject;
	botStatusObject.pData = botStatusDelta;
	botStatusObject.pKey = "botStatusValue";
	botStatusObject.type = SHADOW_JSON_OBJECT;
	botStatusObject.cb = botStatusCallback;

	
	//Register the jsonStruct object
	rc = aws_iot_shadow_register_delta(&mqttClient, &botStatusObject);

	// Now wait in the loop to receive any message sent from the console
	while (rc == NONE_ERROR) {
		/*
		 * Lets check for the incoming messages for 200 ms.
		 */
		rc = aws_iot_shadow_yield(&mqttClient, 200);

		if (messageArrivedOnDelta) {
			INFO("\nSending delta message back %s\n", botStatusDelta);

			printf("\nbotStatusValue:%d\n\n", botStatusValue);
			move(fd);

			rc = aws_iot_shadow_update(&mqttClient, AWS_IOT_MY_THING_NAME, botStatusDelta, UpdateStatusCallback, NULL, 2, true);
			messageArrivedOnDelta = false;
		}

		// sleep for some time in seconds
		sleep(0.1);
	}

	if (NONE_ERROR != rc) {
		ERROR("An error occurred in the loop %d", rc);
	}

	INFO("Disconnecting");
	rc = aws_iot_shadow_disconnect(&mqttClient);

	if (NONE_ERROR != rc) {
		ERROR("Disconnect error %d", rc);
	}

	return rc;
}
bool buildJSONForReported(char *pJsonDocument, size_t maxSizeOfJsonDocument, const char *pReceivedDeltaData, uint32_t lengthDelta) {
	int32_t ret;

	if (pJsonDocument == NULL) {
		return false;
	}

	char tempClientTokenBuffer[MAX_SIZE_CLIENT_TOKEN_CLIENT_SEQUENCE];

	if(aws_iot_fill_with_client_token(tempClientTokenBuffer, MAX_SIZE_CLIENT_TOKEN_CLIENT_SEQUENCE) != NONE_ERROR){
		return false;
	}

	ret = snprintf(pJsonDocument, maxSizeOfJsonDocument, "{\"state\":{\"reported\":%.*s}, \"clientToken\":\"%s\"}", lengthDelta, pReceivedDeltaData, tempClientTokenBuffer);

	if (ret >= maxSizeOfJsonDocument || ret < 0) {
		return false;
	}

	return true;
}

void parseInputArgsForConnectParams(int argc, char** argv) {
	int opt;

	while (-1 != (opt = getopt(argc, argv, "h:p:c:"))) {
		switch (opt) {
		case 'h':
			strcpy(HostAddress, optarg);
			DEBUG("Host %s", optarg);
			break;
		case 'p':
			port = atoi(optarg);
			DEBUG("arg %s", optarg);
			break;
		case 'c':
			strcpy(certDirectory, optarg);
			DEBUG("cert root directory %s", optarg);
			break;
		case '?':
			if (optopt == 'c') {
				ERROR("Option -%c requires an argument.", optopt);
			} else if (isprint(optopt)) {
				WARN("Unknown option `-%c'.", optopt);
			} else {
				WARN("Unknown option character `\\x%x'.", optopt);
			}
			break;
		default:
			ERROR("ERROR in command line argument parsing");
			break;
		}
	}

}

void botStatusCallback(const char *pJsonValueBuffer, uint32_t valueLength, jsonStruct_t *pJsonStruct_t) {

	DEBUG("Received Delta message %.*s", valueLength, pJsonValueBuffer);
	//read_value(pJsonValueBuffer, valueLength);
	botStatusValue = atoi(pJsonValueBuffer);
	printf("buttonstatus = %d\n", botStatusValue);

	if (buildJSONForReported(botStatusDelta, SHADOW_MAX_SIZE_OF_RX_BUFFER, pJsonValueBuffer, valueLength)) {
		messageArrivedOnDelta = true;
	}

}

void UpdateStatusCallback(const char *pThingName, ShadowActions_t action, Shadow_Ack_Status_t status,
		const char *pReceivedJsonDocument, void *pContextData) {

	if (status == SHADOW_ACK_TIMEOUT) {
		INFO("Update Timeout--");
	} else if (status == SHADOW_ACK_REJECTED) {
		INFO("Update RejectedXX");
	} else if (status == SHADOW_ACK_ACCEPTED) {
		INFO("Update Accepted !!");
	}
}

void move(int fd){
	if (botStatusValue == stop){
		printf("stop received, sending \"s6t\"");
		fflush (stdout);
		serialPutchar(fd, 's');
		fflush (stdout);
		serialPutchar(fd, '6');
		fflush (stdout);
		serialPutchar(fd, 't');
	}
	if (botStatusValue == forward){
		printf("forward received, sending \"s1t\"");
		fflush (stdout);
		serialPutchar(fd, 's');
		fflush (stdout);
		serialPutchar(fd, '1');
		fflush (stdout);
		serialPutchar(fd, 't');
	}
	if (botStatusValue == left){
		printf("left received, sending \"s3t\"");
		serialPutchar(fd, 's');
		serialPutchar(fd, '3');
		serialPutchar(fd, 't');
	}
	if (botStatusValue == backward){
		printf("backward received, sending \"s2t\"");
		serialPutchar(fd, 's');
		serialPutchar(fd, '2');
		serialPutchar(fd, 't');
	}
	if (botStatusValue == right){
		printf("right received, sending \"s4t\"");
		serialPutchar(fd, 's');
		serialPutchar(fd, '4');
		serialPutchar(fd, 't');
	}
	if (botStatusValue == standup){
		printf("up received, sending \"s6t\"");
		serialPutchar(fd, 's');
		serialPutchar(fd, '6');
		serialPutchar(fd, 't');
	}
	if (botStatusValue == sitdown){
		printf("down received, sending \"s7t\"");
		serialPutchar(fd, 's');
		serialPutchar(fd, '7');
		serialPutchar(fd, 't');
	}
}
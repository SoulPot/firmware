#include "AzureIoT.h"
#include <azure_ca.h>
SensorsManager* AzureIot::sensorsManager = nullptr;
const char* AzureIot::host = IOT_CONFIG_IOTHUB_FQDN;
const char* AzureIot::device_id = IOT_CONFIG_DEVICE_ID;
const char* AzureIot::mqtt_broker_uri = "mqtts://" IOT_CONFIG_IOTHUB_FQDN;
int AzureIot::mqtt_port = AZ_IOT_DEFAULT_MQTT_CONNECT_PORT;
esp_mqtt_client_handle_t  AzureIot::mqtt_client;
az_iot_hub_client AzureIot::client;
char AzureIot::mqtt_client_id[128];
char AzureIot::mqtt_username[128];
char AzureIot::mqtt_password[200];
char AzureIot::incoming_data[INCOMING_DATA_BUFFER_SIZE];
uint8_t AzureIot::sas_signature_buffer[256];
AzIoTSasToken* AzureIot::sasToken = nullptr;
unsigned long AzureIot::next_data_send_time_ms = 0;
uint32_t AzureIot::payload_send_count = 0;
uint8_t AzureIot::payload[PAYLOAD_SIZE];
struct tm* AzureIot::ptm = nullptr;
char AzureIot::topic[TOPIC_SIZE];

void AzureIot::init(SensorsManager* sensorsManager) {
    AzureIot::sensorsManager = sensorsManager;
}

void AzureIot::initializeTime() {
    Logger.Info("Setting time using SNTP");
    configTime(GMT_OFFSET_SECS, GMT_OFFSET_SECS_DST, NTP_SERVERS);
    time_t now = time(NULL);
    while (now < UNIX_TIME_NOV_13_2017)
    {
        delay(500);
        Serial.print(".");
        now = time(nullptr);
    }
    Serial.println("");
    Logger.Info("Time initialized!");
}

void AzureIot::receivedCallback(char* topic, byte* payload, unsigned int length) {
    Logger.Info("Received [");
    Logger.Info(topic);
    Logger.Info("]: ");
    for (int i = 0; i < length; i++)
    {
        Serial.print((char)payload[i]);
    }
    Serial.println("");
}

esp_err_t AzureIot::mqtt_event_handler(esp_mqtt_event_handle_t event) {
    switch (event->event_id)
    {
        int i, r;

        case MQTT_EVENT_ERROR:
            Logger.Info("MQTT event MQTT_EVENT_ERROR");
        break;
            case MQTT_EVENT_CONNECTED:
            Logger.Info("MQTT event MQTT_EVENT_CONNECTED");

            r = esp_mqtt_client_subscribe(mqtt_client, AZ_IOT_HUB_CLIENT_C2D_SUBSCRIBE_TOPIC, 1);
            if (r == -1)
            {
                Logger.Error("Could not subscribe for cloud-to-device messages.");
            }
            else
            {
                Logger.Info("Subscribed for cloud-to-device messages; message id:"  + String(r));
            }

        break;
            case MQTT_EVENT_DISCONNECTED:
            Logger.Info("MQTT event MQTT_EVENT_DISCONNECTED");
        break;
            case MQTT_EVENT_SUBSCRIBED:
            Logger.Info("MQTT event MQTT_EVENT_SUBSCRIBED");
        break;
            case MQTT_EVENT_UNSUBSCRIBED:
            Logger.Info("MQTT event MQTT_EVENT_UNSUBSCRIBED");
        break;
            case MQTT_EVENT_PUBLISHED:
            Logger.Info("MQTT event MQTT_EVENT_PUBLISHED");
        break;
            case MQTT_EVENT_DATA:
            Logger.Info("MQTT event MQTT_EVENT_DATA");

            for (i = 0; i < (INCOMING_DATA_BUFFER_SIZE - 1) && i < event->topic_len; i++)
            {
                incoming_data[i] = event->topic[i]; 
            }
            incoming_data[i] = '\0';
            Logger.Info("Topic: " + String(incoming_data));
            
            for (i = 0; i < (INCOMING_DATA_BUFFER_SIZE - 1) && i < event->data_len; i++)
            {
                incoming_data[i] = event->data[i]; 
            }
            incoming_data[i] = '\0';
            Logger.Info("Data: " + String(incoming_data));
            incomingDataFilter();
            break;
        case MQTT_EVENT_BEFORE_CONNECT:
            Logger.Info("MQTT event MQTT_EVENT_BEFORE_CONNECT");
            break;
        default:
            Logger.Error("MQTT event UNKNOWN");
            break;
    }

    return ESP_OK;
}

void AzureIot::incomingDataFilter() {
    std::string data(incoming_data);
    if (data.find(",") != std::string::npos) {
        std::string prop1 = data.substr(1, data.find(",") - 1);
        std::string prop2 = data.substr(prop1.length() + 2, data.length() - 2);
        prop1 = prop1.substr(1, prop1.length() - 2);
        prop2 = prop2.substr(1, prop2.length() - 2);
        std::string val1 = prop1.substr(prop1.find(":") + 2, prop1.length());
        prop1 = prop1.substr(0, prop1.find(":") - 1);

        std::string val2 = prop2.substr(prop2.find(":") + 2, prop2.length() - 1);
        val2 = val2.substr(0, val2.length() - 1);
        prop2 = prop2.substr(0, prop2.find(":") - 1);

        int size = 2;
        std::string props[size] = {prop1, prop2};
        std::string values[size] = {val1, val2};

        int value = 0;
        bool sprinkle;
        for (int i = 0; i < size; i++) {
            if (props[i] == "sprinkle") {
                sprinkle = values[i] == "true" ? true : false;
            } else if (props[i] == "expectedValue") {
                value = std::stoi(values[i]);
            } else if (props[i] == "reset") {
                if (values[i] == "true") ESPManager::reset();
            }
        }
        if (value != 0) {
            sensorsManager->sprink(sprinkle, value);
        }
    } else {
        // TODO: faire une payload en mode "action: "reset || restart"
        std::string propName = data.substr(2, data.find(":") - 2);
        std::string propVal = data.substr(data.find(":") + 2, data.length() - 2);
        propName = propName.substr(0, propName.length() - 1);
        propVal = propVal.substr(0, propVal.length() - 2);
        if (propName == "reset" && propVal == "true") {
            ESPManager::reset();
            ESPManager::restart();
        } else if (propName == "restart" && propVal == "true") {
            ESPManager::restart();
        }
    }
}

void AzureIot::initializeIoTHubClient() {
    az_iot_hub_client_options options = az_iot_hub_client_options_default();
    options.user_agent = AZ_SPAN_FROM_STR(AZURE_SDK_CLIENT_USER_AGENT);

    if (az_result_failed(az_iot_hub_client_init(
            &AzureIot::client,
            az_span_create((uint8_t*)AzureIot::host, strlen(AzureIot::host)),
            az_span_create((uint8_t*)AzureIot::device_id, strlen(AzureIot::device_id)),
            &options)))
    {
        Logger.Error("Failed initializing Azure IoT Hub client");
        return;
    }

    size_t client_id_length;
    if (az_result_failed(az_iot_hub_client_get_client_id(
            &AzureIot::client, AzureIot::mqtt_client_id, sizeof(AzureIot::mqtt_client_id) - 1, &client_id_length)))
    {
        Logger.Error("Failed getting client id");
        return;
    }

    if (az_result_failed(az_iot_hub_client_get_user_name(
            &AzureIot::client, AzureIot::mqtt_username, sizeofarray(AzureIot::mqtt_username), NULL)))
    {
        Logger.Error("Failed to get MQTT clientId, return code");
        return;
    }

    Logger.Info("Client ID: " + String(AzureIot::mqtt_client_id));
    Logger.Info("Username: " + String(AzureIot::mqtt_username));
}

int AzureIot::initializeMqttClient() {
    if (sasToken->Generate(SAS_TOKEN_DURATION_IN_MINUTES) != 0)
    {
        Logger.Error("Failed generating SAS token");
        return 1;
    }

    esp_mqtt_client_config_t mqtt_config;
    memset(&mqtt_config, 0, sizeof(mqtt_config));
    mqtt_config.uri = mqtt_broker_uri;
    mqtt_config.port = mqtt_port;
    mqtt_config.client_id = mqtt_client_id;
    mqtt_config.username = mqtt_username;
    mqtt_config.password = (const char*)az_span_ptr(sasToken->Get());
    mqtt_config.keepalive = 30;
    mqtt_config.disable_clean_session = 0;
    mqtt_config.disable_auto_reconnect = false;
    mqtt_config.event_handle = mqtt_event_handler;
    mqtt_config.user_context = NULL;
    mqtt_config.cert_pem = (const char*)ca_pem;

    mqtt_client = esp_mqtt_client_init(&mqtt_config);

    if (mqtt_client == NULL)
    {
        Logger.Error("Failed creating mqtt client");
        return 1;
    }

    esp_err_t start_result = esp_mqtt_client_start(mqtt_client);

    if (start_result != ESP_OK)
    {
        Logger.Error("Could not start mqtt client; error code:" + start_result);
        return 1;
    }
    else
    {
        Logger.Info("MQTT client started");
        return 0;
    }
}

uint32_t AzureIot::getEpochTimeInSecs() {
    return (uint32_t)time(NULL);
}

void AzureIot::destroyMqtt() {
    (void)esp_mqtt_client_destroy(mqtt_client);
}

void AzureIot::establishConnection() {
    AzureIot::sasToken = new AzIoTSasToken(&client, AZ_SPAN_FROM_STR(IOT_CONFIG_DEVICE_KEY),AZ_SPAN_FROM_BUFFER(sas_signature_buffer), AZ_SPAN_FROM_BUFFER(mqtt_password));
    initializeTime();
    initializeIoTHubClient();
    (void)initializeMqttClient();
}

void AzureIot::getPayload(az_span payload, az_span* out_payload) {
    az_span original_payload = payload;
    payload = az_span_copy(
        payload, AZ_SPAN_FROM_STR("{ \"msgCount\": "));
    (void)az_span_u32toa(payload, payload_send_count++, &payload);
    payload = az_span_copy(
        payload, AZ_SPAN_FROM_STR(",\"device_id\": \""));
    payload = az_span_copy(
        payload, AZ_SPAN_FROM_STR(IOT_CONFIG_DEVICE_ID));
    payload = az_span_copy(
        payload, AZ_SPAN_FROM_STR("\""));
    payload = az_span_copy(payload, constructDataPayload()); 
    payload = az_span_copy(payload, AZ_SPAN_FROM_STR(" }"));
    payload = az_span_copy_u8(payload, '\0');

    *out_payload = az_span_slice(original_payload, 0, az_span_size(original_payload) - az_span_size(payload));
}

az_span AzureIot::constructDataPayload() {
    time_t now = time(NULL);
    ptm = gmtime(&now);
    // Logger.Debug(isotime(ptm));
    double temp = sensorsManager->getTemperature();
    double lum = sensorsManager->getLux();
    int moisture = sensorsManager->getMoisture();
    std::string payloadStr = ",\"data\": {\n \"temp\": \"" + std::to_string(temp) + "\",\"hygro\": \""+ std::to_string(moisture) + "\",\"lum\": \"" + std::to_string(lum) + "\"\n },";
    Logger.Debug(payloadStr.c_str());
    az_span span = az_span_create_from_str((char*) payloadStr.c_str());
    return span;
}

void AzureIot::sendData() {
    if (ESPManager::getGlobalState() == ANALYZER_ERROR) {
        return;
    }
    az_span data = AZ_SPAN_FROM_BUFFER(payload);
    if (az_result_failed(az_iot_hub_client_telemetry_get_publish_topic(
            &client, NULL, topic, sizeof(topic), NULL)))
    {
        Logger.Error("Failed az_iot_hub_client_telemetry_get_publish_topic");
        return;
    }
    Logger.Info("Payload construction: in progress...");
    getPayload(data, &data);
    Logger.Info("Payload construction: success");
    if (esp_mqtt_client_publish(
            mqtt_client,
            topic,
            (const char*)az_span_ptr(data),
            az_span_size(data),
            MQTT_QOS1,
            DO_NOT_RETAIN_MSG)
        == 0)
    {
        Logger.Error("Failed publishing");
    }
    else
    {
        Logger.Info("Message published successfully");
    }
    next_data_send_time_ms = millis() + 15000;
}

bool AzureIot::tokenExpired() {
    if (AzureIot::sasToken != nullptr) {
        return AzureIot::sasToken->IsExpired();
    }
    return false;
}

bool AzureIot::time2send() {
    return millis() > AzureIot::next_data_send_time_ms;
}
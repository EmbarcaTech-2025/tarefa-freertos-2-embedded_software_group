#ifndef MQTT_COMM_H
#define MQTT_COMM_H


typedef void (*CallbackFunction)(void *arg, const u8_t *data, u16_t len, u8_t flags);

typedef struct {
    CallbackFunction function;
    char *mqtt_topic;
} Subscriber_Data;

// connect function 
void mqtt_setup_publish(const char *client_id, const char *broker_ip, const char *user, const char *pass);

// publish function
void mqtt_comm_publish(const char *topic, const uint8_t *data, size_t len);

// subscribe functions
void mqtt_setup_and_subscribe(const char *client_id, const char *broker_ip, const char *user, const char *pass, Subscriber_Data *arguments_to_subscriber);


#endif
#include "lwip/apps/mqtt.h"       // Biblioteca MQTT do lwIP
#include "include/mqtt_comm.h"    // Header file com as declarações locais
// Base: https://github.com/BitDogLab/BitDogLab-C/blob/main/wifi_button_and_led/lwipopts.h
#include "lwipopts.h"             // Configurações customizadas do lwIP
#include "include/xor_cipher.h"

//#define MQTT_BROKER_IP "192.168.151.142"
//#define MQTT_PORT 1883
//#define MQTT_TOPIC "escola/sala1/temperatura"

/* Variável global estática para armazenar a instância do cliente MQTT
 * 'static' limita o escopo deste arquivo */
static mqtt_client_t *client;



static void mqtt_pub_request_cb(void *arg, err_t result) {
    if (result == ERR_OK) {
        printf("Publicação MQTT enviada com sucesso!\n");
    } else {
        printf("Erro ao publicar via MQTT: %d\n", result);
    }
}

void mqtt_comm_publish(const char *topic, const uint8_t *data, size_t len) {
    // Envia a mensagem MQTT
    err_t status = mqtt_publish(
        client,              // Instância do cliente
        topic,               // Tópico de publicação
        data,                // Dados a serem enviados
        len,                 // Tamanho dos dados
        0,                   // QoS 0 (nenhuma confirmação)
        0,                   // Não reter mensagem
        mqtt_pub_request_cb, // Callback de confirmação
        NULL                 // Argumento para o callback
    );

    if (status != ERR_OK) {
        printf("mqtt_publish falhou ao ser enviada: %d\n", status);
    }
}

/* Callback de conexão MQTT - chamado quando o status da conexão muda
 * Parâmetros:
 *   - client: instância do cliente MQTT
 *   - arg: argumento opcional (não usado aqui)
 *   - status: resultado da tentativa de conexão */
static void mqtt_connection_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status) {
    if (status == MQTT_CONNECT_ACCEPTED) {
        printf("Conectado ao broker MQTT com sucesso!\n");
    } else {
        printf("Falha ao conectar ao broker, código: %d\n", status);
    }
}

/* Função para configurar e iniciar a conexão MQTT
 * Parâmetros:
 *   - client_id: identificador único para este cliente
 *   - broker_ip: endereço IP do broker como string (ex: "192.168.1.1")
 *   - user: nome de usuário para autenticação (pode ser NULL)
 *   - pass: senha para autenticação (pode ser NULL) */
void mqtt_setup_publish(const char *client_id, const char *broker_ip, const char *user, const char *pass) {
    ip_addr_t broker_addr;  // Estrutura para armazenar o IP do broker
    
    // Converte o IP de string para formato numérico
    if (!ip4addr_aton(broker_ip, &broker_addr)) {
        printf("Erro no IP\n");
        return;
    }

    // Cria uma nova instância do cliente MQTT
    client = mqtt_client_new();
    if (client == NULL) {
        printf("Falha ao criar o cliente MQTT\n");
        return;
    }

    // Configura as informações de conexão do cliente
    struct mqtt_connect_client_info_t ci = {
        .client_id = client_id,  // ID do cliente
        .client_user = user,     // Usuário (opcional)
        .client_pass = pass      // Senha (opcional)
    };

    // Inicia a conexão com o broker
    // Parâmetros:
    //   - client: instância do cliente
    //   - &broker_addr: endereço do broker
    //   - 1883: porta padrão MQTT
    //   - mqtt_connection_cb: callback de status
    //   - NULL: argumento opcional para o callback
    //   - &ci: informações de conexão
    mqtt_client_connect(client, &broker_addr, 1883, mqtt_connection_cb, NULL, &ci);
}


// subscribe functions
//static struct mqtt_client mqtt_client_instance;
//static ip_addr_t broker_ip;
uint32_t ultima_timestamp_recebida = 0;

void on_message(char* mqtt_topic, char* mqtt_msg) {
    // 1. Parse do JSON (exemplo simplificado)
    uint32_t nova_timestamp;
    float valor;
    if (sscanf(mqtt_msg, "{\"valor\":%f,\"ts\":%lu}", &valor, &nova_timestamp) != 2) {
        printf("Erro no parse da mensagem!\n");
        return;
    }

    // 2. Verificação de replay
    if (nova_timestamp > ultima_timestamp_recebida) {
        ultima_timestamp_recebida = nova_timestamp;
        printf("Nova leitura: %.2f (ts: %lu)\n", valor, nova_timestamp);
        
        // --> Processar dados aqui <--
        
    } else {
        printf("Replay detectado (ts: %lu <= %lu)\n", 
               nova_timestamp, ultima_timestamp_recebida);
    }
}


static void mqtt_incoming_publish_cb(void *arg, const char *topic, u32_t tot_len) {
    printf("Received message on topic: %s\n", topic);
}

/*
static void mqtt_incoming_data_cb(void *arg, const u8_t *data, u16_t len, u8_t flags) {
    uint8_t descriptografada[101];

    
    //xor_encrypt((uint8_t *)data, descriptografada, strlen(data), 42);

    //printf("Payload: %.*s\n", len, descriptografada);
   //on_message((char*)arg, descriptografada);
    
    gpio_put(led_verde, data[0]);


}
*/

static void mqtt_connection_cb_and_subscribe(mqtt_client_t *client, void *arg, mqtt_connection_status_t status) {
    if (status == MQTT_CONNECT_ACCEPTED) {
        char *mqtt_topic = (char *)arg;
        printf("MQTT connected!\n");

        err_t err = mqtt_subscribe(client, mqtt_topic, 0, NULL, NULL);
        if (err == ERR_OK) {
            printf("Subscribed to topic: %s\n", mqtt_topic);
        } else {
            printf("Failed to subscribe, error: %d\n", err);
        }
    } else {
        printf("MQTT connection failed with status: %d\n", status);
    }
}

void mqtt_setup_and_subscribe(const char *client_id, const char *broker_ip, const char *user, const char *pass, Subscriber_Data *arguments_to_subscriber) {
    ip_addr_t broker_addr;  // Estrutura para armazenar o IP do broker
    void *arg = arguments_to_subscriber->mqtt_topic;
    
    // Converte o IP de string para formato numérico
    if (!ip4addr_aton(broker_ip, &broker_addr)) {
        printf("Erro no IP\n");
        return;
    }

    // Cria uma nova instância do cliente MQTT
    client = mqtt_client_new();
    if (client == NULL) {
        printf("Falha ao criar o cliente MQTT\n");
        return;
    }

    // Configura as informações de conexão do cliente
    struct mqtt_connect_client_info_t ci = {
        .client_id = client_id,  // ID do cliente
        .client_user = user,     // Usuário (opcional)
        .client_pass = pass      // Senha (opcional)
    };

    mqtt_set_inpub_callback(client, mqtt_incoming_publish_cb, 
        arguments_to_subscriber->function, 
        (void*)arguments_to_subscriber->mqtt_topic);


    // Inicia a conexão com o broker
    // Parâmetros:
    //   - client: instância do cliente
    //   - &broker_addr: endereço do broker
    //   - 1883: porta padrão MQTT
    //   - mqtt_connection_cb: callback de status
    //   - NULL: argumento opcional para o callback
    //   - &ci: informações de conexão
    mqtt_client_connect(client, &broker_addr, 1883, mqtt_connection_cb_and_subscribe, arg, &ci);
}





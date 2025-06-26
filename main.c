// C standard headers
#include <stdio.h>
#include <string.h>                 // Para funções de string como strlen()
#include <time.h>
#include <string.h>

// PICO Standard headers
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"        // Driver WiFi para Pico W
#include "hardware/i2c.h"

// RTOS headres
#include "FreeRTOS.h"
#include "task.h"

// project specific headers
#include "include/wifi_conn.h"      // Funções personalizadas de conexão WiFi
#include "include/mqtt_comm.h"      // Funções personalizadas para MQTT
#include "include/xor_cipher.h"     // Funções de cifra XOR

// global variable for the handles of the tasks
TaskHandle_t xLedTaskHandle = NULL;


uint led_verde = 11;
uint with_cryptography = 1;

void start_gpios() {
    gpio_init(led_verde);
    gpio_set_dir(led_verde, GPIO_OUT);
    gpio_put(led_verde, 0);
}


void mqtt_incoming_data_cb(void *arg, const u8_t *data, u16_t len, u8_t flags) {
    uint8_t descriptografada[101];
    uint led_status;

    if (with_cryptography) {
        xor_encrypt((uint8_t *)data, descriptografada, strlen(data), 42);
        //printf("Payload: %.*s\n", len, descriptografada);
        on_message((char*)arg, descriptografada);


    } else {
        printf("Payload: %s\n", data);
        on_message((char*)arg, data);

    }


    if (data[0] == '1') {
        led_status = 1;
    }
    else if (data[0] == '0') {
        led_status = 0;
    }

    gpio_put(led_verde, led_status);


}


void led_mqtt() {
      // variaveis de inicializacao
    uint is_subscriber = 1;
    uint is_publisher = 0;
    const char* mqtt_topic = "bitdoglab/atuadores/led_verde"; 
    //const char* mqtt_topic = "escola/sala1/temperatura";
    uint xor_key = 42;
    const char *IP = "192.168.15.145";
    const char *USER = "aluno";
    const char *USER_PASSWORD = "senha123";
    char client_id[31];
    char client_subscriber[] = "bitdog_subscriber";
    char client_publisher[] = "bitdog_publisher";
    char SSID[] = "VIVOFIBRA-5598";
    char SSID_PASSWORD[] = "4674B29BC2";

    // inicia gpios 
    start_gpios();

    // Conecta à rede WiFi
    // Parâmetros: Nome da rede (SSID)R e senha
    connect_to_wifi(SSID, SSID_PASSWORD);

    // Configura o cliente MQTT
    // Parâmetros: ID do cliente, IP do broker, usuário, senha
    if (is_subscriber) {
        //mqtt_setup_and_subscribe("bitdog_subscriber", "192.168.151.142", "aluno", "senha123");
        strcpy(client_id, client_subscriber);

        Subscriber_Data arguments_to_subscriber = {
            .function = mqtt_incoming_data_cb,
            .mqtt_topic = mqtt_topic
        };


        mqtt_setup_and_subscribe(client_id, IP, USER, USER_PASSWORD, &arguments_to_subscriber);
    } 
    if (is_publisher) {
        //mqtt_setup_publish("bitdog_publisher", "192.168.151.142", "aluno", "senha123");
        strcpy(client_id, client_publisher);
        mqtt_setup_publish(client_id, IP, USER, USER_PASSWORD);
    }
    
    // Mensagem original a ser enviada
    uint8_t mensagem[101];

   
    // Loop principal do programa
    while (true) {
        if (is_publisher) {
            // Dados a serem enviados
            float payload = 31.2;
    
            time_t seconds = time(NULL);

            sprintf(mensagem, "{\"valor\":%.2f,\"ts\":%lu}", payload, seconds);
            printf("Mensagem enviada: %s\n", mensagem);

            // Publica a mensagem original (não criptografada)
            //mqtt_comm_publish("escola/sala1/temperatura", mensagem, strlen(mensagem));
            //printf("A mensagem %s foi enviada !!!\n", mensagem);
             // Buffer para mensagem criptografada (16 bytes)
            
            if (with_cryptography) {
                uint8_t criptografada[101];
                // Criptografa a mensagem usando XOR com chave 42
                xor_encrypt((uint8_t *)mensagem, criptografada, strlen(mensagem), xor_key);

                
                // Alternativa: Publica a mensagem criptografada (atualmente comentada)
                mqtt_comm_publish(mqtt_topic, criptografada, strlen(mensagem));
            } else {
                mqtt_comm_publish(mqtt_topic, mensagem, strlen(mensagem));
            }

            // wait 5 seconds before the next publishing
            sleep_ms(5000);
            
        }
    }
    
    
}

int main() {
  stdio_init_all();

  xTaskCreate(led_mqtt, "LED MQTT", 256, NULL, 1, &xLedTaskHandle);

  vTaskStartScheduler();

  while(1){};

  return 0;
}
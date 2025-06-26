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

// usuario e senha para usar o MQTT
const char *USER = "aluno";
const char *USER_PASSWORD = "senha123";

// information for authentication on the network
char SSID[] = "VIVOFIBRA-5598";
char SSID_PASSWORD[] = "4674B29BC2";


uint led_green = 11;
uint led_red = 13;
uint led_blue = 12;
uint with_cryptography = 0;

void set_led(uint color_red, uint color_green, uint color_blue) {
  gpio_put(led_red, color_red);
  gpio_put(led_green, color_green);
  gpio_put(led_blue, color_blue);
}

typedef struct {
  uint color_red;
  uint color_blue;
  uint color_green;

} Led_t;

Led_t leds = {0,0,0};

void start_gpios() {
    gpio_init(led_green);
    gpio_set_dir(led_green, GPIO_OUT);
    gpio_put(led_green, 0);

    gpio_init(led_red);
    gpio_set_dir(led_red, GPIO_OUT);
    gpio_put(led_red, 0);

    gpio_init(led_blue);
    gpio_set_dir(led_blue, GPIO_OUT);
    gpio_put(led_blue, 0);
}


void mqtt_incoming_data_cb_leds(void *arg, const u8_t *data, u16_t len, u8_t flags) {
    uint8_t descriptografada[101];
    uint led_status;
    char payload[len+1];

    strncpy(payload, (const char*)data, len);
    payload[len] = '\0';

    if (with_cryptography) {
        xor_encrypt((uint8_t *)data, descriptografada, strlen(data), 42);
        printf("Payload: %.*s\n", len, descriptografada);
        //on_message((char*)arg, descriptografada);


    } else {
      printf("Payload: %s\n", payload);
    }

    if (strcmp(payload, "green_on") == 0) {
        leds.color_green = 1;
        printf("ok");
    }
    if (strcmp(payload, "blue_on") == 0) {
      leds.color_blue = 1;
    }

    if (strcmp(payload, "red_on") == 0) {
      leds.color_red = 1;
    }

    if (strcmp(payload, "green_off") == 0) {
        leds.color_green = 0;
    }
    if (strcmp(payload, "blue_off") == 0) {
      leds.color_blue = 0;
    }

    if (strcmp(payload, "red_off") == 0) {
      leds.color_red = 0;
    }



    set_led(leds.color_red, leds.color_green, leds.color_blue);

}


void led_mqtt() {
      // variaveis de inicializacao
    uint is_subscriber = 1;
    uint is_publisher = 0;
    const char* mqtt_topic = "bitdoglab/atuadores/leds"; 
    uint xor_key = 42;
    const char *IP = "192.168.15.145";
    char client_id[31];
    char client_subscriber[] = "bitdog_subscriber";
    char client_publisher[] = "bitdog_publisher";

    // Conecta à rede WiFi
    // Parâmetros: Nome da rede (SSID)R e senha
    connect_to_wifi(SSID, SSID_PASSWORD);

    // Configura o cliente MQTT
    // Parâmetros: ID do cliente, IP do broker, usuário, senha
    if (is_subscriber) {
        strcpy(client_id, client_subscriber);

        Subscriber_t arguments_to_subscriber = {
            .function = mqtt_incoming_data_cb_leds,
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
      vTaskDelay(pdMS_TO_TICKS(100)); // Polling a cada 100ms
    }
    

}

int main() {
  stdio_init_all();
  
  // inicia gpios 
  start_gpios();

  xTaskCreate(led_mqtt, "LED MQTT", 256, NULL, 1, &xLedTaskHandle);

  vTaskStartScheduler();

  while(1){};

  return 0;
}
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/pbuf.h"
#include "lwip/tcp.h"

// Configurações de rede
const char *ssid = "CLARO_102"; // SSID da minha Rede
const char *password = "Eduardo8080@"; // Senha da minha rede

// Buffer para as requisições HTTP
char request_buffer[2048];
int request_length = 0;

// Definição das LEDS que serão indicativas
#define LED_VERDE 11
#define LED_AZUL 12
#define LED_VERMELHA 13

// Função para extrair dados recebidos em formato JSON
char *find_json_value(const char *json, const char *key) {
    char *key_start = strstr(json, key);
    if (key_start == NULL) {
        return NULL;
    }

    char *value_start = strchr(key_start, ':') + 1; 
    if (value_start == NULL) {
        return NULL;
    }

    while (*value_start == ' ') {
        value_start++;
    }

    char *value_end;
    if (*value_start == '"') {
        value_start++;
        value_end = strchr(value_start, '"');
    } else {
        value_end = strchr(value_start, ',');
        if (value_end == NULL) {
            value_end = strchr(value_start, '}'); 
        }
    }

    if (value_end == NULL) {
        return NULL; 
    }

    int value_len = value_end - value_start;
    char *value = malloc(value_len + 1);
    strncpy(value, value_start, value_len);
    value[value_len] = '\0';

    return value;
}

// Função para configurar as leds dado sinais vitais
void control_leds(int verde, int azul, int vermelha) {
    gpio_put(LED_VERDE, verde);
    gpio_put(LED_AZUL, azul);
    gpio_put(LED_VERMELHA, vermelha);
}

// Classificando os sinais vitais
void categorize_vital_signs(int heart_rate, int systolic, int diastolic, int oxygen) {
    if ((heart_rate >= 60 && heart_rate <= 80) && 
        (systolic >= 110 && systolic <= 120) && 
        (diastolic >= 70 && diastolic <= 80) && 
        (oxygen >= 97 && oxygen <= 100)) {
        printf("Sinais vitais normais.\n");
        control_leds(1, 0, 0); // Acendendo a LED verde
    } else if ((heart_rate >= 81 && heart_rate <= 100) || 
               (systolic >= 121 && systolic <= 130) || 
               (diastolic >= 81 && diastolic <= 90) || 
               (oxygen >= 94 && oxygen <= 96)) {
        printf("Sinais vitais levemente alterados.\n");
        control_leds(0, 1, 0); // Acendendo a LED azul
    } else {
        printf("Sinais vitais críticos! Atenção necessária!\n");
        control_leds(0, 0, 1); // Acendendo a LED vermelha
    }
}

// Calback para recebimento via http POST
err_t http_server_recv(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err) {
    if (p == NULL) {
        tcp_close(pcb);
        return ERR_OK;
    }

    pbuf_copy_partial(p, request_buffer + request_length, p->tot_len, 0);
    request_length += p->tot_len;
    request_buffer[request_length] = '\0';
    pbuf_free(p);

    char *body = strstr(request_buffer, "{");
    if (body) {
        char *heart_rate_str = find_json_value(body, "heart_rate");
        char *systolic_str = find_json_value(body, "systolic_pressure");
        char *diastolic_str = find_json_value(body, "diastolic_pressure");
        char *oxygen_str = find_json_value(body, "oxygen_saturation");

        if (heart_rate_str && systolic_str && diastolic_str && oxygen_str) {
            int heart_rate = atoi(heart_rate_str);
            int systolic = atoi(systolic_str);
            int diastolic = atoi(diastolic_str);
            int oxygen = atoi(oxygen_str);

            categorize_vital_signs(heart_rate, systolic, diastolic, oxygen);

            free(heart_rate_str);
            free(systolic_str);
            free(diastolic_str);
            free(oxygen_str);
        } else {
            printf("Erro ao analisar valores JSON\n");
        }
    }

    request_length = 0;
    memset(request_buffer, 0, sizeof(request_buffer));
    return ERR_OK;
}

// Calback de aceitar requisição http
err_t http_server_accept(void *arg, struct tcp_pcb *pcb, err_t err) {
    if (err == ERR_OK && pcb != NULL) {
        tcp_recv(pcb, http_server_recv);
    }
    return ERR_OK;
}

int main() {
    stdio_init_all();
    sleep_ms(5000);
    printf("Inicializando servidor...\n");

    // Configura os GPIOs dos LEDs
    gpio_init(LED_VERDE);
    gpio_init(LED_AZUL);
    gpio_init(LED_VERMELHA);
    gpio_set_dir(LED_VERDE, GPIO_OUT);
    gpio_set_dir(LED_AZUL, GPIO_OUT);
    gpio_set_dir(LED_VERMELHA, GPIO_OUT);

    // Apagando todas as leds
    control_leds(0, 0, 0);

    if (cyw43_arch_init()) {
        printf("Falha ao inicializar Wi-Fi\n");
        return -1;
    }

    cyw43_arch_enable_sta_mode();
    printf("Conectando ao Wi-Fi...\n");

    if (cyw43_arch_wifi_connect_timeout_ms(ssid, password, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        printf("Falha ao conectar ao Wi-Fi\n");
        return -1;
    } else {
        printf("Conectado ao Wi-Fi! Servidor rodando na porta 80\n");
    }

    struct tcp_pcb *pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);
    if (!pcb) return -1;
    if (tcp_bind(pcb, IP_ANY_TYPE, 80) != ERR_OK) return -1;
    pcb = tcp_listen(pcb);
    if (!pcb) return -1;
    tcp_accept(pcb, http_server_accept);

    while (true) {
        cyw43_arch_poll();
        sleep_ms(1000);
    }
    return 0;
}
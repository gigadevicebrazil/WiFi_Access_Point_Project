/*!
    \file    main.c
    \brief   Main loop of GD32VW55x SDK.

    \version 2023-07-20, V1.0.0, firmware for GD32VW55x
*/

/*
    Bloco de licença da GigaDevice.

    Esse trecho define as condições de uso, redistribuição e responsabilidade
    do código fornecido pela fabricante. Não interfere na execução do programa,
    mas deve ser mantido quando o código for redistribuído.
*/

/* Bibliotecas padrão da linguagem C */
#include <stdint.h>     /* Tipos inteiros com tamanho fixo: uint8_t, uint32_t etc. */
#include <stdio.h>      /* Funções de entrada/saída, como printf e snprintf */
#include <string.h>     /* Funções de string/memória: strlen, strncmp, strchr etc. */
#include <errno.h>      /* Variável errno para identificar erros de socket */

/* Arquivos do SDK e da plataforma GD32VW55x */
#include "app_cfg.h"                    /* Configurações gerais do projeto */
#include "gd32vw55x_platform.h"         /* Inicialização e abstrações da plataforma */
#include "gd32vw55x.h"                  /* Definições específicas do microcontrolador */

/* Bibliotecas de socket do LwIP */
#include "lwip/sockets.h"               /* API de sockets: socket, bind, listen, accept, send, recv */
#include "lwip/priv/sockets_priv.h"     /* Definições internas/privadas de sockets do LwIP */

/* Bibliotecas de Wi-Fi do SDK */
#include "wifi_management.h"            /* API de alto nível para Wi-Fi: SoftAP, STA, scan etc. */
#include "wifi_init.h"                  /* Inicialização do módulo Wi-Fi */

/* Utilitários do SDK */
#include "util.h"                       /* Inicialização de utilitários do sistema */
#include "user_setting.h"               /* Inicialização/carregamento de configurações do usuário */


/* ===================== User configuration ===================== */

/*
    Nome da rede Wi-Fi que será criada pela placa em modo SoftAP.

    Quando o firmware rodar, o celular ou notebook verá uma rede chamada:
    GD32-LED_SeuNome

    Você pode alterar esse texto para personalizar o treinamento.
*/
#define SOFTAP_SSID            "GD32-LED_SeuNome"

/*
    Senha da rede Wi-Fi SoftAP.

    Para WPA2/WPA3, a senha precisa ter no mínimo 8 caracteres.
    Se quiser criar uma rede aberta, use string vazia:
    #define SOFTAP_PASSWORD ""
*/
#define SOFTAP_PASSWORD        "12345678"

/*
    Canal Wi-Fi usado pelo SoftAP.

    Valores típicos: 1 a 13.
    No exemplo foi usado o canal 11.
*/
#define SOFTAP_CHANNEL         11

/*
    Define se o SSID será oculto.

    0 = rede visível
    1 = rede oculta
*/
#define SOFTAP_HIDDEN          0

/*
    Porta TCP onde o servidor HTTP irá escutar.

    Porta 80 é a porta padrão de HTTP.
    Assim o usuário pode acessar:
    http://192.168.4.1/
*/
#define HTTP_LISTEN_PORT       80

/*
    Tamanho do buffer usado para receber a requisição HTTP.

    O servidor implementado é simples e espera requisições pequenas,
    por isso 512 bytes são suficientes para comandos como:
    GET /led1/on HTTP/1.1
*/
#define HTTP_RECV_BUF_SZ       512


/* ===================== LED configuration ===================== */

/*
    Configuração do LED1.

    LED1 está ligado ao pino PB0:
    - Porta GPIOB
    - Pino GPIO_PIN_0
    - Clock da porta RCU_GPIOB
*/
#define LED1_PORT              GPIOB
#define LED1_PIN               GPIO_PIN_0
#define LED1_RCU               RCU_GPIOB

/*
    Configuração do LED2.

    LED2 está ligado ao pino PA12:
    - Porta GPIOA
    - Pino GPIO_PIN_12
    - Clock da porta RCU_GPIOA
*/
#define LED2_PORT              GPIOA
#define LED2_PIN               GPIO_PIN_12
#define LED2_RCU               RCU_GPIOA

/*
    Configuração do LED3.

    LED3 está ligado ao pino PB4:
    - Porta GPIOB
    - Pino GPIO_PIN_4
    - Clock da porta RCU_GPIOB
*/
#define LED3_PORT              GPIOB
#define LED3_PIN               GPIO_PIN_4
#define LED3_RCU               RCU_GPIOB

/*
    Ajuste de polaridade dos LEDs.

    Algumas placas ligam o LED quando o GPIO vai para nível alto.
    Outras ligam o LED quando o GPIO vai para nível baixo.

    LED_ACTIVE_LOW = 0:
        LED liga com nível alto.
        gpio_bit_set() liga.
        gpio_bit_reset() desliga.

    LED_ACTIVE_LOW = 1:
        LED liga com nível baixo.
        gpio_bit_reset() liga.
        gpio_bit_set() desliga.
*/
#define LED_ACTIVE_LOW         0


/* ===================== Minimal HTML page ===================== */

/*
    Página HTML servida pelo microcontrolador.

    Esta string contém uma página web completa:
    - HTML
    - CSS
    - JavaScript

    O navegador acessa essa página em:
    http://192.168.4.1/

    Cada botão chama uma rota HTTP diferente usando fetch(), por exemplo:
    /led1/on
    /led1/off
    /all/on
    /status
*/
static const char k_index_html[] =
"<!doctype html><html><head><meta charset='utf-8'>"
"<meta name='viewport' content='width=device-width,initial-scale=1'>"
"<title>GD32 LED Wi-Fi</title>"

/* Estilos visuais da página */
"<style>"
"body{font-family:system-ui,Segoe UI,Roboto,Arial;max-width:720px;margin:24px;background:#f7f7f7;}"
"h1{font-size:28px;}"
".card{background:white;padding:20px;border-radius:16px;box-shadow:0 2px 12px #0001;margin-bottom:16px;}"
"button{font-size:18px;padding:14px 18px;margin:6px;border:0;border-radius:10px;background:#2563eb;color:white;}"
"button.off{background:#555;}"
"button.all{background:#16a34a;}"
"button.demo{background:#9333ea;}"
".s{color:#555;margin-top:12px;}"
"code{background:#eee;padding:2px 6px;border-radius:6px;}"
"</style>"
"</head><body>"

/* Título principal */
"<h1>GD32VW553 - Controle Wi-Fi de LEDs</h1>"

/* Card de controle do LED1 */
"<div class='card'>"
"<h2>LED1 - PB0</h2>"
"<button onclick=\"cmd('/led1/on')\">LED1 ON</button>"
"<button class='off' onclick=\"cmd('/led1/off')\">LED1 OFF</button>"
"</div>"

/* Card de controle do LED2 */
"<div class='card'>"
"<h2>LED2 - PA12</h2>"
"<button onclick=\"cmd('/led2/on')\">LED2 ON</button>"
"<button class='off' onclick=\"cmd('/led2/off')\">LED2 OFF</button>"
"</div>"

/* Card de controle do LED3 */
"<div class='card'>"
"<h2>LED3 - PB4</h2>"
"<button onclick=\"cmd('/led3/on')\">LED3 ON</button>"
"<button class='off' onclick=\"cmd('/led3/off')\">LED3 OFF</button>"
"</div>"

/* Card de controle geral */
"<div class='card'>"
"<h2>Controle geral</h2>"
"<button class='all' onclick=\"cmd('/all/on')\">Todos ON</button>"
"<button class='off' onclick=\"cmd('/all/off')\">Todos OFF</button>"
"<button class='demo' onclick=\"cmd('/demo')\">Demo</button>"
"<button onclick=\"statusLed()\">Status</button>"
"<div class='s'>Resposta: <code id='resp'>--</code></div>"
"</div>"

/*
    JavaScript da página.

    A função cmd(path):
    - envia uma requisição HTTP GET para a rota indicada
    - lê a resposta em texto
    - mostra a resposta no campo "resp"

    A função statusLed():
    - acessa /status
    - recebe JSON
    - mostra o estado dos LEDs
*/
"<script>"
"async function cmd(path){"
"  try{"
"    const r=await fetch(path,{cache:'no-store'});"
"    const t=await r.text();"
"    document.getElementById('resp').textContent=t;"
"  }catch(e){"
"    document.getElementById('resp').textContent='Erro de conexao';"
"  }"
"}"
"async function statusLed(){"
"  try{"
"    const r=await fetch('/status',{cache:'no-store'});"
"    const j=await r.json();"
"    document.getElementById('resp').textContent='LED1='+j.led1+' LED2='+j.led2+' LED3='+j.led3;"
"  }catch(e){"
"    document.getElementById('resp').textContent='Erro de conexao';"
"  }"
"}"
"</script>"
"</body></html>";


/* ===================== LED helpers ===================== */

/*
    Variáveis globais que armazenam o estado lógico dos LEDs.

    0 = desligado
    1 = ligado

    Essas variáveis representam o estado desejado.
    A função leds_apply() copia esses estados para os pinos físicos.
*/
static uint8_t led1_state = 0U;
static uint8_t led2_state = 0U;
static uint8_t led3_state = 0U;

/*
    Escreve em um pino de LED respeitando a polaridade configurada.

    Parâmetros:
    gpio_periph: porta GPIO, por exemplo GPIOA ou GPIOB
    pin: pino da porta, por exemplo GPIO_PIN_0
    on: 1 para ligar, 0 para desligar
*/
static void led_write(uint32_t gpio_periph, uint32_t pin, uint8_t on)
{
#if LED_ACTIVE_LOW

    /*
        Caso LED_ACTIVE_LOW = 1:
        - LED liga com nível baixo
        - LED desliga com nível alto
    */
    if (on) {
        gpio_bit_reset(gpio_periph, pin);
    } else {
        gpio_bit_set(gpio_periph, pin);
    }

#else

    /*
        Caso LED_ACTIVE_LOW = 0:
        - LED liga com nível alto
        - LED desliga com nível baixo
    */
    if (on) {
        gpio_bit_set(gpio_periph, pin);
    } else {
        gpio_bit_reset(gpio_periph, pin);
    }

#endif
}

/*
    Aplica os estados armazenados nas variáveis globais aos pinos físicos.

    Essa função centraliza a escrita dos três LEDs.
*/
static void leds_apply(void)
{
    led_write(LED1_PORT, LED1_PIN, led1_state);
    led_write(LED2_PORT, LED2_PIN, led2_state);
    led_write(LED3_PORT, LED3_PIN, led3_state);
}

/*
    Inicializa os GPIOs dos LEDs.

    Etapas:
    1. Habilita o clock das portas GPIO.
    2. Configura os pinos como saída digital.
    3. Configura saída push-pull.
    4. Define velocidade de saída.
    5. Inicializa todos os LEDs desligados.
*/
static void leds_init(void)
{
    /* Habilita clock das portas GPIO usadas pelos LEDs */
    rcu_periph_clock_enable(LED1_RCU);
    rcu_periph_clock_enable(LED2_RCU);
    rcu_periph_clock_enable(LED3_RCU);

    /* Configura LED1 como saída */
    gpio_mode_set(LED1_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LED1_PIN);
    gpio_output_options_set(LED1_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_10MHZ, LED1_PIN);

    /* Configura LED2 como saída */
    gpio_mode_set(LED2_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LED2_PIN);
    gpio_output_options_set(LED2_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_10MHZ, LED2_PIN);

    /* Configura LED3 como saída */
    gpio_mode_set(LED3_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LED3_PIN);
    gpio_output_options_set(LED3_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_10MHZ, LED3_PIN);

    /* Estado inicial: todos desligados */
    led1_state = 0U;
    led2_state = 0U;
    led3_state = 0U;

    /* Aplica o estado inicial nos pinos */
    leds_apply();
}

/*
    Desliga todos os LEDs.
*/
static void leds_all_off(void)
{
    led1_state = 0U;
    led2_state = 0U;
    led3_state = 0U;
    leds_apply();
}

/*
    Liga todos os LEDs.
*/
static void leds_all_on(void)
{
    led1_state = 1U;
    led2_state = 1U;
    led3_state = 1U;
    leds_apply();
}

/*
    Compara o caminho recebido na requisição HTTP com uma rota esperada.

    Exemplo:
    path = "/led1/on"
    expected = "/led1/on"

    Retorna:
    1 = caminho é igual ao esperado
    0 = caminho é diferente
*/
static int path_is(const char *path, size_t path_len, const char *expected)
{
    size_t expected_len = strlen(expected);

    return ((path_len == expected_len) &&
            (strncmp(path, expected, expected_len) == 0));
}


/* ===================== HTTP helpers ===================== */

/*
    Envia todos os bytes de um buffer por um socket.

    A função send() nem sempre envia tudo de uma vez.
    Por isso esta função fica em loop até enviar todos os bytes.

    Parâmetros:
    fd: descritor do socket
    buf: ponteiro para os dados
    len: quantidade de bytes a enviar

    Retorna:
    0 = sucesso
    -1 = falha
*/
static int send_all(int fd, const void *buf, size_t len)
{
    const uint8_t *p = (const uint8_t *)buf;

    while (len > 0) {
        int n = send(fd, p, (int)len, 0);

        if (n <= 0) {
            return -1;
        }

        p += (size_t)n;
        len -= (size_t)n;
    }

    return 0;
}

/*
    Envia uma resposta HTTP completa.

    Esta função monta:
    - status HTTP
    - Content-Type
    - Content-Length
    - Cache-Control
    - Connection close

    Depois envia o cabeçalho e o corpo da resposta.

    Exemplo de resposta:
    HTTP/1.1 200 OK
    Content-Type: text/plain; charset=utf-8
    Content-Length: 11
    Cache-Control: no-store
    Connection: close

    OK LED1 ON
*/
static void http_send_response(int fd,
                               const char *status,
                               const char *content_type,
                               const char *body,
                               size_t body_len)
{
    char hdr[256];

    int hdr_len = snprintf(
        hdr,
        sizeof(hdr),
        "HTTP/1.1 %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %u\r\n"
        "Cache-Control: no-store\r\n"
        "Connection: close\r\n"
        "\r\n",
        status,
        content_type,
        (unsigned)body_len
    );

    /* Se snprintf falhar, não tenta enviar resposta */
    if (hdr_len < 0) {
        return;
    }

    /* Envia cabeçalho HTTP */
    (void)send_all(fd, hdr, (size_t)hdr_len);

    /* Envia corpo da resposta, se existir */
    if (body && body_len) {
        (void)send_all(fd, body, body_len);
    }
}

/*
    Atalho para enviar texto puro com status 200 OK.
*/
static void http_send_text(int fd, const char *body)
{
    http_send_response(fd,
                       "200 OK",
                       "text/plain; charset=utf-8",
                       body,
                       strlen(body));
}

/*
    Trata uma conexão HTTP recebida.

    Essa função:
    1. Recebe a requisição HTTP.
    2. Verifica se o método é GET.
    3. Extrai o caminho da URL.
    4. Executa a ação correspondente.
    5. Envia a resposta ao navegador.
*/
static void http_handle_client(int cli_fd)
{
    char req[HTTP_RECV_BUF_SZ];

    /* Zera o buffer antes de receber dados */
    sys_memset(req, 0, sizeof(req));

    /*
        Recebe a requisição do cliente.

        Exemplo esperado:
        GET /led1/on HTTP/1.1
        Host: 192.168.4.1
        ...
    */
    int n = recv(cli_fd, req, (int)(sizeof(req) - 1U), 0);

    if (n <= 0) {
        return;
    }

    /*
        Parser HTTP mínimo.

        O servidor aceita somente requisições GET.
        POST, PUT, DELETE etc. recebem erro 405.
    */
    if (strncmp(req, "GET ", 4) != 0) {
        static const char body[] = "Method Not Allowed\n";

        http_send_response(cli_fd,
                           "405 Method Not Allowed",
                           "text/plain; charset=utf-8",
                           body,
                           sizeof(body) - 1U);
        return;
    }

    /*
        O caminho começa logo após "GET ".

        Exemplo:
        req  = "GET /led1/on HTTP/1.1"
        path = "/led1/on HTTP/1.1"
    */
    const char *path = req + 4;

    /*
        Procura o espaço depois do caminho.

        Em:
        GET /led1/on HTTP/1.1

        O espaço depois de /led1/on indica o fim do path.
    */
    const char *sp = strchr(path, ' ');

    if (!sp) {
        static const char body[] = "Bad Request\n";

        http_send_response(cli_fd,
                           "400 Bad Request",
                           "text/plain; charset=utf-8",
                           body,
                           sizeof(body) - 1U);
        return;
    }

    /* Calcula o tamanho do caminho */
    size_t path_len = (size_t)(sp - path);

    /*
        Rota principal "/".

        Quando o navegador acessa:
        http://192.168.4.1/

        O firmware retorna a página HTML de controle dos LEDs.
    */
    if (path_is(path, path_len, "/")) {
        http_send_response(cli_fd,
                           "200 OK",
                           "text/html; charset=utf-8",
                           k_index_html,
                           strlen(k_index_html));
        return;
    }

    /*
        Rota: /led1/on

        Liga o LED1.
    */
    if (path_is(path, path_len, "/led1/on")) {
        led1_state = 1U;
        leds_apply();

        printf("HTTP: LED1 ON\r\n");

        http_send_text(cli_fd, "OK LED1 ON\n");
        return;
    }

    /*
        Rota: /led1/off

        Desliga o LED1.
    */
    if (path_is(path, path_len, "/led1/off")) {
        led1_state = 0U;
        leds_apply();

        printf("HTTP: LED1 OFF\r\n");

        http_send_text(cli_fd, "OK LED1 OFF\n");
        return;
    }

    /*
        Rota: /led2/on

        Liga o LED2.
    */
    if (path_is(path, path_len, "/led2/on")) {
        led2_state = 1U;
        leds_apply();

        printf("HTTP: LED2 ON\r\n");

        http_send_text(cli_fd, "OK LED2 ON\n");
        return;
    }

    /*
        Rota: /led2/off

        Desliga o LED2.
    */
    if (path_is(path, path_len, "/led2/off")) {
        led2_state = 0U;
        leds_apply();

        printf("HTTP: LED2 OFF\r\n");

        http_send_text(cli_fd, "OK LED2 OFF\n");
        return;
    }

    /*
        Rota: /led3/on

        Liga o LED3.
    */
    if (path_is(path, path_len, "/led3/on")) {
        led3_state = 1U;
        leds_apply();

        printf("HTTP: LED3 ON\r\n");

        http_send_text(cli_fd, "OK LED3 ON\n");
        return;
    }

    /*
        Rota: /led3/off

        Desliga o LED3.
    */
    if (path_is(path, path_len, "/led3/off")) {
        led3_state = 0U;
        leds_apply();

        printf("HTTP: LED3 OFF\r\n");

        http_send_text(cli_fd, "OK LED3 OFF\n");
        return;
    }

    /*
        Rota: /all/on

        Liga todos os LEDs.
    */
    if (path_is(path, path_len, "/all/on")) {
        leds_all_on();

        printf("HTTP: ALL ON\r\n");

        http_send_text(cli_fd, "OK ALL ON\n");
        return;
    }

    /*
        Rota: /all/off

        Desliga todos os LEDs.
    */
    if (path_is(path, path_len, "/all/off")) {
        leds_all_off();

        printf("HTTP: ALL OFF\r\n");

        http_send_text(cli_fd, "OK ALL OFF\n");
        return;
    }

    /*
        Rota: /demo

        Executa uma sequência visual:
        1. LED1 liga
        2. LED2 liga
        3. LED3 liga
        4. todos ligam
        5. todos desligam

        Cada etapa espera 300 ms.
    */
    if (path_is(path, path_len, "/demo")) {
        printf("HTTP: DEMO\r\n");

        led1_state = 1U;
        led2_state = 0U;
        led3_state = 0U;
        leds_apply();
        sys_ms_sleep(300);

        led1_state = 0U;
        led2_state = 1U;
        led3_state = 0U;
        leds_apply();
        sys_ms_sleep(300);

        led1_state = 0U;
        led2_state = 0U;
        led3_state = 1U;
        leds_apply();
        sys_ms_sleep(300);

        led1_state = 1U;
        led2_state = 1U;
        led3_state = 1U;
        leds_apply();
        sys_ms_sleep(300);

        leds_all_off();

        http_send_text(cli_fd, "OK DEMO\n");
        return;
    }

    /*
        Rota: /status

        Retorna o estado atual dos LEDs em formato JSON.

        Exemplo:
        {"led1":1,"led2":0,"led3":1}
    */
    if (path_is(path, path_len, "/status")) {
        char body[96];

        int bl = snprintf(
            body,
            sizeof(body),
            "{\"led1\":%u,\"led2\":%u,\"led3\":%u}\n",
            (unsigned)led1_state,
            (unsigned)led2_state,
            (unsigned)led3_state
        );

        if (bl < 0) {
            bl = 0;
        }

        http_send_response(cli_fd,
                           "200 OK",
                           "application/json; charset=utf-8",
                           body,
                           (size_t)bl);
        return;
    }

    /*
        Alguns navegadores pedem automaticamente /favicon.ico.

        Para evitar erro desnecessário, o firmware responde 204 No Content.
    */
    if (path_is(path, path_len, "/favicon.ico")) {
        http_send_response(cli_fd,
                           "204 No Content",
                           "text/plain",
                           NULL,
                           0);
        return;
    }

    /*
        Qualquer rota desconhecida retorna 404.
    */
    static const char body[] = "Not Found\n";

    http_send_response(cli_fd,
                       "404 Not Found",
                       "text/plain; charset=utf-8",
                       body,
                       sizeof(body) - 1U);
}

/*
    Loop principal do servidor HTTP.

    Essa função:
    1. Cria um socket TCP.
    2. Permite reutilização de endereço.
    3. Faz bind na porta 80.
    4. Coloca o socket em modo listen.
    5. Aceita clientes em loop infinito.
    6. Trata cada cliente.
    7. Fecha a conexão.
*/
static void http_server_loop(void)
{
    int listen_fd = -1;
    struct sockaddr_in addr;

    /*
        Cria um socket TCP IPv4.

        AF_INET: IPv4
        SOCK_STREAM: TCP
        protocolo 0: protocolo padrão para TCP
    */
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (listen_fd < 0) {
        printf("HTTP: socket() failed (%d)\r\n", errno);
        return;
    }

    /*
        Permite reutilizar o endereço/porta.

        Útil caso o servidor seja reiniciado rapidamente.
    */
    int reuse = 1;

    (void)setsockopt(listen_fd,
                     SOL_SOCKET,
                     SO_REUSEADDR,
                     (const char *)&reuse,
                     sizeof(reuse));

    /*
        Prepara o endereço local do servidor.

        INADDR_ANY:
        aceita conexões em qualquer IP local da interface.

        Porta:
        HTTP_LISTEN_PORT, neste caso 80.
    */
    sys_memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_len = sizeof(addr);
    addr.sin_port = htons(HTTP_LISTEN_PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    /*
        Associa o socket à porta local.

        Se falhar, normalmente é porque:
        - a porta já está em uso
        - a interface ainda não está pronta
        - houve erro interno no LwIP
    */
    if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        printf("HTTP: bind() failed (%d)\r\n", errno);
        goto out;
    }

    /*
        Coloca o socket em modo servidor.

        O segundo parâmetro, 4, é o backlog:
        número aproximado de conexões pendentes permitidas.
    */
    if (listen(listen_fd, 4) != 0) {
        printf("HTTP: listen() failed (%d)\r\n", errno);
        goto out;
    }

    printf("HTTP server listening on port %u\r\n",
           (unsigned)HTTP_LISTEN_PORT);

    /*
        Loop infinito do servidor.

        O firmware fica aqui aguardando conexões HTTP.
    */
    while (1) {
        struct sockaddr_in cli_addr;
        socklen_t len = sizeof(cli_addr);

        /*
            Aguarda uma conexão de cliente.

            Quando o navegador acessa a página ou aperta um botão,
            uma conexão TCP é aceita aqui.
        */
        int cli_fd = accept(listen_fd,
                            (struct sockaddr *)&cli_addr,
                            &len);

        if (cli_fd < 0) {
            /*
                EAGAIN indica tentativa sem conexão disponível.
                Nesse caso apenas continua aguardando.
            */
            if (errno == EAGAIN) {
                continue;
            }

            printf("HTTP: accept() failed (%d)\r\n", errno);
            continue;
        }

        /*
            Trata a requisição HTTP desse cliente.
        */
        http_handle_client(cli_fd);

        /*
            Fecha a conexão após responder.

            Isso simplifica bastante o servidor:
            uma conexão = uma requisição = uma resposta.
        */
        shutdown(cli_fd, SHUT_RDWR);
        close(cli_fd);
    }

out:
    /*
        Fecha o socket principal caso ocorra erro de inicialização.
    */
    if (listen_fd >= 0) {
        shutdown(listen_fd, SHUT_RDWR);
        close(listen_fd);
    }
}


/* ===================== SoftAP task ===================== */

/*
    Task principal da aplicação Wi-Fi + HTTP + LED.

    Essa função roda dentro de uma task do RTOS.
    Ela:
    1. Configura SSID, senha, canal e modo de autenticação.
    2. Inicia o SoftAP.
    3. Inicializa os LEDs.
    4. Inicia o servidor HTTP.
*/
static void softap_http_led_task(void *param)
{
    (void)param;

    /*
        Cria buffers locais para SSID e senha.

        A API wifi_management_ap_start() recebe char*.
        Por isso são usadas arrays locais modificáveis,
        não diretamente literais const.
    */
    char ssid[] = SOFTAP_SSID;
    char password_buf[] = SOFTAP_PASSWORD;
    char *password = password_buf;

    uint8_t channel = SOFTAP_CHANNEL;
    uint8_t is_hidden = SOFTAP_HIDDEN;

    /*
        Modo padrão de autenticação.

        AUTH_MODE_WPA2_WPA3:
        permite autenticação WPA2/WPA3, dependendo do suporte do cliente.
    */
    wifi_ap_auth_mode_t auth_mode = AUTH_MODE_WPA2_WPA3;

    /*
        Verifica se o SSID está vazio.

        SoftAP precisa de um nome de rede válido.
    */
    if (ssid[0] == '\0') {
        printf("SoftAP: SSID cannot be empty\r\n");
        goto exit;
    }

    /*
        Se a senha for string vazia, transforma em NULL.

        password == NULL indica rede aberta.
    */
    if (password && (strlen(password) == 0U)) {
        password = NULL;
    }

    /*
        Se não houver senha, usa autenticação aberta.
    */
    if (password == NULL) {
        auth_mode = AUTH_MODE_OPEN;
    }

    /*
        Inicia o SoftAP usando a API do Wi-Fi Management.

        Parâmetros:
        ssid: nome da rede
        password: senha ou NULL
        channel: canal Wi-Fi
        auth_mode: modo de autenticação
        is_hidden: 0 visível, 1 oculto
    */
    printf("Start Wi-Fi softap...\r\n");

    int ret = wifi_management_ap_start(ssid,
                                       password,
                                       channel,
                                       auth_mode,
                                       is_hidden);

    if (ret != 0) {
        printf("SoftAP start failed (%d)\r\n", ret);
        goto exit;
    }

    /*
        Após iniciar o SoftAP, o celular pode se conectar à rede criada.
    */
    printf("SoftAP '%s' started.\r\n", ssid);
    printf("Connect with your phone and open: http://192.168.4.1/\r\n");
    printf("(If it does not open, use the Gateway IP shown by the phone)\r\n");

    /*
        Inicializa os LEDs depois do Wi-Fi estar ativo.
    */
    leds_init();

    printf("LEDs ready: LED1=PB0, LED2=PA12, LED3=PB4.\r\n");

    /*
        Inicia o servidor HTTP.

        Esta função entra em loop infinito, portanto normalmente
        o código abaixo dela só será executado se houver erro ou saída futura.
    */
    http_server_loop();

    /*
        Para o SoftAP se o servidor HTTP encerrar.
    */
    wifi_management_ap_stop();

exit:
    /*
        Finaliza a task caso ocorra erro ou encerramento.
    */
    printf("Demo ended.\r\n");
    sys_task_delete(NULL);
}


/*!
    \brief      Init applications.
                This function is called to initialize all the applications.
    \param[in]  none.
    \param[out] none.
    \retval     none.
*/
static void application_init(void)
{
    /*
        Inicialização opcional do shell de comandos.

        Esse bloco só será compilado se alguma dessas macros estiver habilitada:
        - CONFIG_BASECMD
        - CONFIG_RF_TEST_SUPPORT
        - CONFIG_BLE_DTM_SUPPORT

        O shell permite executar comandos via console/serial,
        dependendo da configuração do SDK.
    */
#if defined CONFIG_BASECMD || defined CONFIG_RF_TEST_SUPPORT || defined CONFIG_BLE_DTM_SUPPORT
    if (cmd_shell_init()) {
        dbg_print(ERR, "cmd shell init failed\r\n");
    }
#endif

    /*
        Inicialização do modo AT Command, se habilitado.

        CONFIG_ATCMD permite controlar funcionalidades via comandos AT.
    */
#ifdef CONFIG_ATCMD
    if (atcmd_init()) {
        dbg_print(ERR, "atcmd init failed\r\n");
    }
#endif

    /*
        Inicializa utilitários internos do SDK.
    */
    util_init();

    /*
        Inicializa configurações persistentes/de usuário do SDK.
    */
    user_setting_init();

    /*
        Inicialização opcional do BLE.

        Se CFG_BLE_SUPPORT estiver habilitado, o firmware inclui suporte BLE.

        CONFIG_BLE_ALWAYS_ENABLE:
        - se definido, inicia BLE imediatamente com ble_init(true)
        - senão, inicializa de forma não ativa com ble_init(false)
    */
#ifdef CFG_BLE_SUPPORT
#ifdef CONFIG_BLE_ALWAYS_ENABLE
    ble_init(true);
#else
    ble_init(false);
#endif
#endif

    /*
        Inicialização do Wi-Fi.

        Esse bloco só é compilado se CFG_WLAN_SUPPORT estiver habilitado.

        wifi_init():
        - inicializa PMU do Wi-Fi
        - inicializa módulos internos do Wi-Fi
        - prepara driver, stack e gerenciamento

        Se wifi_init() retornar 0, cria a task da aplicação SoftAP.
    */
#ifdef CFG_WLAN_SUPPORT
    if (wifi_init()) {
        dbg_print(ERR, "wifi init failed\r\n");
    } else {
        /*
            Cria dinamicamente a task "softap http led".

            Parâmetros:
            nome da task: "softap http led"
            stack: 4096 bytes
            prioridade: OS_TASK_PRIORITY(1)
            função da task: softap_http_led_task
            parâmetro: NULL
        */
        (void)sys_task_create_dynamic(
            (const uint8_t *)"softap http led",
            4096,
            OS_TASK_PRIORITY(1),
            softap_http_led_task,
            NULL
        );
    }

    /*
        lcd_logo_task_start() está comentado.
        Provavelmente era usado em algum exemplo com display.
    */
    // lcd_logo_task_start();
#endif

    /*
        Inicialização opcional do FatFS.

        Se CONFIG_FATFS_SUPPORT estiver habilitado e CONFIG_ATCMD não estiver,
        monta/cria sistema de arquivos.
    */
#ifdef CONFIG_FATFS_SUPPORT
#ifndef CONFIG_ATCMD
    fatfs_mk_mount(NULL);
#endif
#endif

    /*
        Inicialização opcional do Matter.
    */
#ifdef CFG_MATTER
    MatterInit();
#endif

    /*
        Inicialização opcional de demo Azure.
    */
#ifdef CONFIG_AZURE_F527_DEMO_SUPPORT
    azure_task_start();
#endif
}


#ifdef PLATFORM_OS_RTTHREAD
/*!
    \brief      Start task.
                This function is called to initialize all the applications in thread context.
    \param[in]  param parameter passed to the task
    \param[out] none
    \retval     none.
*/
static void start_task(void *param)
{
    (void)param;

    /*
        Em RT-Thread, a inicialização da aplicação é feita dentro
        de uma task específica.
    */
    application_init();

    /*
        Após inicializar a aplicação, esta task se encerra.
    */
    sys_task_delete(NULL);
}
#endif


/*!
    \brief      Main entry point.
                This function is called right after the booting process has completed.
    \param[in]  none.
    \param[out] none.
    \retval     none.
*/
int main(void)
{
    /*
        Inicializa o sistema operacional/RTOS.

        Deve ser chamado antes de criar tasks e iniciar o scheduler.
    */
    sys_os_init();

    /*
        Inicializa a plataforma:
        - clock
        - board support package
        - drivers básicos
        - UART/debug, dependendo do SDK
    */
    platform_init();

#ifdef PLATFORM_OS_RTTHREAD

    /*
        Se estiver usando RT-Thread, cria uma task inicial chamada "start_task".

        Essa task chama application_init().
    */
    if (sys_task_create_dynamic((const uint8_t *)"start_task",
            START_TASK_STACK_SIZE,
            OS_TASK_PRIORITY(START_TASK_PRIO),
            start_task,
            NULL) == NULL) {
        dbg_print(ERR, "Create start task failed\r\n");
    }

#else

    /*
        Se não for RT-Thread, chama a inicialização diretamente.
    */
    application_init();

#endif

    /*
        Inicia o escalonador do RTOS.

        Depois dessa chamada, as tasks começam a executar.
        A função normalmente não retorna.
    */
    sys_os_start();
}

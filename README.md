# Captura de Tráfego Wi-Fi — ESP32 Sniffer

Firmware ESP-IDF para captura de metadados de quadros Wi-Fi IEEE 802.11 na faixa de **2,4 GHz**, usando o modo promíscuo do ESP32. Este repositório corresponde ao dispositivo de captura do primeiro caso de uso do TCC sobre IoT e bancos de dados no IF Goiano – Campus Urutaí.

## Objetivo

Coletar informações técnicas dos quadros Wi-Fi observados pelo ESP32 e encaminhá-las por UART para outro ESP32, responsável por preparar e publicar as mensagens no broker MQTT.

## Fluxo de funcionamento

1. Inicializa o Wi-Fi do ESP32 em modo de captura promíscua.
2. Percorre os canais configurados para observar quadros na banda de 2,4 GHz.
3. Extrai metadados disponíveis da captura, como RSSI, canal, tamanho do quadro e endereços MAC presentes no cabeçalho.
4. Serializa os dados em mensagens JSON.
5. Envia as mensagens pela UART2 para o ESP32 receptor.

A captura registra **metadados de quadros**, não o conteúdo de aplicações transportado pela rede.

## Metadados coletados

Os campos disponíveis dependem do tipo de quadro recebido e da implementação do firmware. Entre os dados tratados estão:

- intensidade do sinal recebido (**RSSI**);
- canal Wi-Fi;
- tamanho do quadro;
- endereços MAC disponíveis no quadro;
- informações de controle do quadro, como ToDS e FromDS, quando aplicáveis.

Nem todo quadro contém todos os campos. A interpretação dos endereços MAC depende do tipo de quadro IEEE 802.11.

## Comunicação entre os ESP32

A transferência para o receptor é feita por UART2. Na configuração documentada para este projeto:

| Parâmetro | Valor |
|---|---|
| GPIO TX | GPIO 17 |
| GPIO RX | GPIO 16 |
| Baud rate | 921600 |
| Formato da mensagem | JSON |

Ligue TX de um dispositivo ao RX do outro e compartilhe o GND. Confirme a pinagem e a configuração serial nos dois firmwares antes de ligar os dispositivos.

## Requisitos

- Placa ESP32 compatível com a API de captura Wi-Fi usada pelo projeto
- ESP-IDF instalado e configurado
- Cabo USB para gravação e monitoramento
- Segundo ESP32 executando o firmware receptor
- Conexão UART entre os dispositivos

## Compilação e gravação

Execute os comandos abaixo em um terminal configurado com ESP-IDF, na raiz do projeto:

```bash
idf.py set-target esp32
idf.py build
idf.py -p PORTA flash monitor
```

Troque `PORTA` pela porta serial da placa. Se o alvo já estiver configurado, pule o comando `set-target`.

## Integração com o restante do sistema

Este repositório é a etapa de captura. O firmware receptor correspondente está em [sniffer-receptor-caso-uso-1](https://github.com/josevictoraraujorojas/sniffer-receptor-caso-uso-1). O receptor valida e complementa as mensagens recebidas e as encaminha via MQTT, no tópico `wifi`, para processamento pela infraestrutura do TCC.

## Cuidados e limitações

- A captura depende do canal sintonizado e do alcance do sinal de rádio.
- O modo promíscuo pode observar quadros de diferentes dispositivos próximos; use os dados respeitando a legislação, a privacidade e as regras da instituição.
- A UART em alta velocidade exige conexões curtas e configuração serial igual nos dois ESP32.

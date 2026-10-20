# Procedimento experimental — captura de tráfego Wi-Fi

Este firmware transmite os quadros selecionados por UART para o ESP32 receptor. O experimento acompanha o log da captura e o resultado do envio serial.

## 1. Compilar, gravar e observar

No terminal ESP-IDF, com o ESP32 sniffer conectado:

```powershell
idf.py build
idf.py -p COM5 flash monitor
```

Troque `COM5` pela porta correta. Para cada quadro transformado em JSON, procure:

```text
[EXPERIMENTO][UART_ENVIO] bytes=... cliente=... bssid=... payload={...}
```

Se ocorrer problema no envio, procure `[EXPERIMENTO][UART_ENVIO_INCOMPLETO]`. O firmware registra também contadores de quadros capturados, mensagens enviadas à UART e JSONs truncados.

## 2. Verificar o receptor

Abra o monitor serial do segundo ESP32, o receptor do repositório `sniffer-receptor-caso-uso-1`. Procure a linha `[EXPERIMENTO][UART_RECEBIDO]`. O payload deve conter os campos esperados, incluindo `cliente` e `bssid`.

## 3. Conferir o broker e o backend

Capture o tópico MQTT publicado pelo receptor:

```powershell
mosquitto_sub -h HOST_DO_BROKER -p 1883 -t wifi -v
```

No backend do Caso de Uso 1, consulte os logs `[EXPERIMENTO][MQTT_RECEBIDO]`, `[EXPERIMENTO][PROCESSAMENTO_OK]` e `[EXPERIMENTO][INFLUX_PERSISTIDO]`. Compare as contagens para o mesmo intervalo. Como o sniffer depende do tráfego de rádio observado e do canal sintonizado, não considere os quadros capturados como equivalentes a todas as transmissões existentes no ambiente.

## Dados a registrar

Para cada repetição, anote a duração, quadros capturados, mensagens enviadas à UART, mensagens recebidas pelo receptor, mensagens observadas no tópico `wifi`, gravações no InfluxDB e erros. Os logs podem conter endereços MAC; mantenha as capturas de acordo com as regras de privacidade e uso da instituição.

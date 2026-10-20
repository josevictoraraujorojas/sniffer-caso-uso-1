# Procedimento experimental — captura Wi-Fi

## Objetivo
Verificar a conformidade do JSON emitido pelo sniffer e obter uma contagem reproduzível dos registros observados.

## Execução
Compile e grave o firmware com o ESP-IDF. Depois, capture o monitor serial:

    idf.py -p PORTA monitor

Copie a saída para um arquivo texto chamado captura.jsonl. O validador ignora linhas sem JSON e analisa objetos JSON encontrados em cada linha. Também é possível preparar um arquivo com apenas as mensagens JSON emitidas pelo sniffer.

Execute:

    py tests\validar_experimento.py --autoteste
    py tests\validar_experimento.py --arquivo captura.jsonl --saida resultado.json

O script verifica campos type, subtype, rssi, canal, tamanho, to_ds, from_ds, direcao, cliente, bssid e timestamp. Também verifica formato dos endereços MAC, RSSI entre -127 e 0 dBm, canal entre 1 e 13 e tamanho positivo.

## Protocolo sugerido
Faça cinco repetições e registre o tempo de captura, o canal/ambiente observado e a quantidade de quadros válidos. Como a captura depende do tráfego de rádio presente, não confunda quadros vistos pelo sniffer com mensagens MQTT persistidas. Documente as condições ambientais da execução.

## Limitação
O autoteste valida somente o script. A captura real exige o ESP32 e tráfego Wi-Fi disponível; a validação JSON não comprova que todos os quadros transmitidos no ambiente foram capturados.

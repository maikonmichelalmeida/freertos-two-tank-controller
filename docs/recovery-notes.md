# Notas de recuperação

## Origem

O material original consistia em um único sketch Arduino usado em uma atividade acadêmica de Sistemas de Tempo Real. A proposta central — uma planta virtual controlada por tarefas concorrentes — foi mantida.

## Problema funcional encontrado

O vetor de níveis possuía dois elementos, mas a válvula de saída escrevia em `virtual_nivel_tanque[2]`. Esse índice é inválido e poderia corromper outras variáveis globais. A versão recuperada descarrega corretamente o segundo tanque (`tankLevel[1]`).

## Ajustes de robustez

- níveis limitados a 0–100%;
- transferência e descarga limitadas ao volume disponível;
- mistura térmica calculada com o volume anterior do tanque 2;
- temporização segura diante do estouro de `millis()`;
- falha explícita se um recurso do FreeRTOS não puder ser criado.

## O que não foi alterado

- modelo de dois tanques;
- três válvulas (entrada, transferência e saída);
- aquecimento do segundo tanque;
- controle por tarefas periódicas;
- mutex único para proteger o estado global;
- limites de histerese definidos no trabalho original.

## Validação pendente

A recuperação foi revisada em código, mas ainda requer compilação e execução em uma placa ESP32 para confirmar temporização, tamanho de pilha e comportamento térmico no ambiente Arduino utilizado originalmente.

# Controlador de dois tanques com FreeRTOS

Fiz este projeto na disciplina de Sistemas de Tempo Real usando ESP32/Arduino. O programa simula dois tanques, três válvulas e uma resistência. Separei o controle de nível, transferência, temperatura e descarga em tarefas do FreeRTOS e usei um mutex para o estado compartilhado.

## Como organizei o controle

- decomposição do controle em tarefas periódicas do FreeRTOS;
- sincronização de estado compartilhado com mutex;
- controle de nível com histerese;
- controle térmico com histerese;
- simulação discreta da planta e telemetria serial;
- investigação e correção de uma escrita fora dos limites do vetor original.

## Arquitetura

| Tarefa | Responsabilidade | Período |
| --- | --- | ---: |
| `plantUpdateTask` | Atualiza níveis e temperatura da planta simulada | 20 ms |
| `heaterControlTask` | Liga/desliga o aquecedor entre 70 °C e 85 °C | 50 ms |
| `inletControlTask` | Controla a entrada do tanque 1 | 50 ms |
| `transferControlTask` | Controla a transferência entre os tanques | 50 ms |
| `outletControlTask` | Libera produto aquecido pelo tanque 2 | 50 ms |
| `telemetryTask` | Publica o estado no monitor serial | 3 s |

Todas as tarefas acessam a estrutura `plant` por meio do mutex `plantMutex`.

## Melhorias aplicadas na recuperação

O código preserva o modelo e a organização concorrente do trabalho acadêmico. Foram feitos apenas ajustes localizados:

- correção de `tankLevel[2]` para `tankLevel[1]`, removendo acesso fora do vetor de dois tanques;
- uso da subtração de `millis()` para funcionar após o estouro do contador;
- limitação dos níveis ao intervalo físico de 0 a 1;
- transferência limitada ao volume disponível;
- verificação da criação do mutex e das tarefas;
- nomes mais claros e remoção de parâmetros de tarefa não utilizados.

## Execução

Alvo esperado: ESP32 com o framework Arduino e FreeRTOS fornecido pelo core do ESP32.

1. Abra `firmware/two_tank_controller/two_tank_controller.ino` na Arduino IDE.
2. Selecione uma placa ESP32 compatível.
3. Compile e carregue o firmware.
4. Abra o monitor serial em 115200 bit/s.

O projeto usa apenas a simulação interna: não exige sensores ou atuadores externos.

## Verificação automatizada

`make check` compila estaticamente o sketch com interfaces mínimas do Arduino e do FreeRTOS. Essa verificação detecta erros de C++, assinaturas incompatíveis e regressões estruturais sem fingir que substitui a compilação para ESP32 ou o ensaio em hardware.

## Estado e limites

Este é um projeto acadêmico recuperado. A lógica foi revisada estaticamente, mas a versão recuperada ainda não foi revalidada em uma placa física. As constantes da planta são didáticas e não representam um processo industrial calibrado.

## Contexto

Desenvolvido durante a graduação em Engenharia de Computação na UERGS, como exercício de sistemas de tempo real e programação concorrente embarcada.

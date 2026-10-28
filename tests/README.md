# Testes das funções de entrada

Os testes extraem funções reais de um checkout Blender com o patch aplicado e
as compilam com interfaces Android/GHOST/UI simuladas. A validação local usa
Zig 0.13.0 e JDK 17 no Windows. Isso não equivale a compilar o aplicativo ou
testá-lo em Android; o novo caminho de texto ainda não passou pelo build Android.

| Conjunto | Verificações locais aprovadas |
| --- | ---: |
| Entrada nativa Android | 629 |
| Teclado virtual | 45 |
| Ponte de texto Java | 72 |
| Assinaturas e encaminhamento JNI | 6 |

As contagens correspondem ao snapshot testado; confira a saída de cada runner
ao executar uma revisão posterior.

Requisitos: Python 3.10 ou superior, Zig 0.13.0 e JDK 17 para o teste Java/JNI.
Não há pacotes Python externos nem downloads automáticos. Execute da raiz deste
repositório, informando o checkout com o patch já aplicado:

```bash
python tests/run_host_tests.py \
  --source /caminho/blender/intern/ghost/intern/GHOST_SystemAndroid.cc \
  --compiler /caminho/zig
python tests/test_virtual_keyboard.py \
  --source /caminho/blender --zig /caminho/zig
python tests/test_android_text_java.py \
  --source /caminho/blender --jdk /caminho/jdk-17 --zig /caminho/zig
```

`--compiler` recebe o executável **Zig**, pois o runner acrescenta o subcomando
`c++`. No primeiro teste `--source` é um arquivo; nos outros, é a pasta do
checkout. Em Windows use `zig.exe` e coloque caminhos com espaços entre aspas.

No Linux, o teste Java/JNI também aceita GCC ou Clang com C++17 e localiza o JDK
pelo `javac` do PATH, se `--jdk` for omitido:

```bash
python3 tests/test_android_text_java.py --source /caminho/blender --cxx g++
```

O teste Java extrai os métodos reais de ambas as Activities e confere que são
iguais. Executa encaminhamento de caracteres, ações, repetição e exclusão com
objetos Android simulados. `javac -h` gera um header das declarações nativas
reais, contra o qual são compiladas as funções JNI reais de `GHOST_AndroidMain.cc`.
Isso detecta divergências entre `byte[]`/`jbyteArray` e entre os cinco argumentos
de tecla, além de verificar UTF-8 padrão para caracteres suplementares. Não
valida os mapas reais de teclado do Android nem compila a Activity contra API 35.

O teste virtual extrai `wmEventModifierFlag` de `WM_types.hh` e a declaração
correspondente de `wm.hh`. Uma compilação negativa confirma que a antiga
combinação com `uint8_t` é rejeitada; interfaces baseadas apenas em inteiros
não podem esconder novamente esse erro de tipo.

A verificação de integração usa os enums reais de `GHOST_Types.hh` e
`wm_event_types.hh`, a conversão real do Window Manager e sua macro de teclado
para confirmar que `GHOST_kKeyUnknown` vira `EVT_UNKNOWNKEY`, sem descartar o
texto associado por tratá-lo como `EVENT_NONE`.

A cobertura nativa inclui modificadores por lado, ordem dos eventos, fontes
física/virtual/toque, foco, desconexão, fila JNI, dígitos da fileira e numpad,
pontuação, F2, navegação, Unicode de um a quatro bytes, repetição, atalhos
Ctrl/Meta, AltGr e composição/fallback de acentos. A consulta ao layout e os
callbacks Android são simulados; os testes comprovam a lógica de encaminhamento.

Conserve todos os arquivos desta pasta juntos: extrator, configuração JSON,
headers de fixture, casos C++ e scripts. Os headers originais necessários devem
existir no checkout selecionado. Saídas geradas, caches e logs não fazem parte
do repositório; `--generated-dir` permite escolher a pasta temporária do teste
Java/JNI. Nunca use um registro antigo como evidência de uma fonte posterior.

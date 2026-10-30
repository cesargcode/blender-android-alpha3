# Testes das funções de entrada e da caneta Wacom

**Revisão Wacom de 30/10/2026.** A suíte nativa passou em **1.088 verificações** (629 anteriores e 459 de movimento),
e passaram **83 Java e 6 JNI**. Com as **45 do teclado virtual mantido**, já
aprovadas, são **1.222 verificações locais**. A preparação final do pacote
reexecuta todas as suítes sobre as fontes entregues.
Build Android Full, novo APK e teste da correção no aparelho pendentes.
A CTL-4100 selecionada por VID `0x056a`/`0x2d1f` e PID `0x0374` usa ponta esquerda,
inferior central e superior direito. A S Pen mantém botão primário direito e
mouse comum mantém seu mapeamento. O APK previsto é
`blender-alpha3-modificadores-wacom-full-arm64-signed.apk`, ainda não produzido.

`android_motion_assertions.cc` está integrado ao `run_host_tests.py` pela lista
`additional_assertions`; não exige comando adicional. A função de movimento e
seus tipos/constantes são extraídos da fonte real; Android/GHOST são simulados.
Os resultados abaixo pertencem ao snapshot anterior, de **28/10 sem Wacom**.

Os testes extraem funções reais de um checkout Blender com o patch aplicado e
as compilam com interfaces Android/GHOST/UI simuladas. A validação local usa
Zig 0.13.0 e JDK 17 no Windows. As **752 verificações locais passaram**; esses
ensaios com interfaces simuladas não executam o aplicativo no Android.

A [execução 6](https://github.com/cesargcode/blender-android-alpha3/actions/runs/40731216675)
concluiu a compilação Full, Java/DEX/APK e as verificações do pacote, com envio dos
16 arquivos confirmado. Após a inclusão local das duas dependências glTF da
revisão fixada, o pacote completo e o APK final assinado passaram em 610 verificações
estáticas, examinando 267 arquivos ELF, sem falhas, avisos ou itens para revisão.
Assinatura v3 e alinhamento com `zipalign -P 16` também passaram.

O instalador `blender-alpha3-modificadores-full-arm64-signed.apk` é entregue
localmente; o draft do GitHub contém o unsigned original. Seu hash e a distinção
entre os artefatos estão no [guia de compilação](../build/README.md).
No APK anterior, o usuário relatou ponta esquerda, botão inferior direito e
superior sem ação no tablet. A validação ampla das demais funções e o teste
do novo remapeamento continuam pendentes.

| Conjunto anterior de 28/10 | Verificações locais aprovadas |
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
headers de fixture, casos C++ (incluindo `android_motion_assertions.cc`) e scripts. Os headers originais necessários devem
existir no checkout selecionado. Saídas geradas, caches e logs não fazem parte
do repositório; `--generated-dir` permite escolher a pasta temporária do teste
Java/JNI. Nunca use um registro antigo como evidência de uma fonte posterior.

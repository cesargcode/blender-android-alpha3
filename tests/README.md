# Testes das funções de entrada

Os testes extraem funções reais de um checkout Blender com o patch aplicado e
as compilam com interfaces Android/GHOST/UI simuladas. Foram aprovadas **210
verificações de entrada Android e 45 de teclado virtual**, usando **Zig 0.13.0**
no host Windows. Isso não equivale a compilar o aplicativo ou testá-lo em Android.

Requisitos: Python 3 e Zig 0.13.0. Não há pacotes Python externos. Execute da raiz
deste repositório, informando o checkout com o patch já aplicado:

```bash
python tests/run_host_tests.py \
  --source /caminho/blender/intern/ghost/intern/GHOST_SystemAndroid.cc \
  --compiler /caminho/zig
python tests/test_virtual_keyboard.py \
  --source /caminho/blender --zig /caminho/zig
```

`--compiler` recebe o executável **Zig**, pois o runner acrescenta o subcomando
`c++`. No primeiro teste `--source` é um arquivo; no segundo, é a pasta do
checkout. Em Windows use `zig.exe` e coloque caminhos com espaços entre aspas.

Conserve todos os arquivos desta pasta juntos: extrator, configuração JSON,
headers de fixture, casos C++ e ambos os scripts. Saídas geradas, caches e logs
não fazem parte do repositório. Os testes cobrem modificadores por lado, ordem
dos eventos, fontes física/virtual/toque, foco, desconexão e fila JNI.

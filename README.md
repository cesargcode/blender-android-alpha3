# Blender Android Alpha 3: modificadores e digitação física

Patch experimental para `simfeo/blender`, tag `android-alpha-3`, commit
`89479ec817bfe9411ae6ef6f35392d5450e1ad0b`. Corrige a entrega de Shift/Ctrl/Alt/Meta,
a coexistência dos modificadores físicos e virtuais, a limpeza após perda de
foco ou desconexão e a entrega de texto do teclado físico aos campos do Blender.
Mantém a configuração Full e a base gráfica.

**Ainda não há instalador disponível ou comportamento validado no aparelho.**
A [quarta execução](https://github.com/cesargcode/blender-android-alpha3/actions/runs/40695500313)
compilou a configuração Full anterior à correção de texto e montou um APK de
cerca de 272 MB. A verificação final falhou ao procurar `sdkVersion` no relatório
que usava `minSdkVersion`; o APK não foi retido. O verificador foi corrigido e o
workflow atualizado tenta preservar APK e relatórios em uma release de rascunho,
inclusive se a validação falhar. As novas alterações de texto ainda precisam de
compilação Android, assinatura e teste no aparelho.

- [Patch](blender-alpha3-modificadores.patch): nove arquivos alterados sobre a base fixada.
- [Receita de compilação](build/README.md): Ubuntu 24.04 x64, saída unsigned por padrão.
- [Testes](tests/README.md): 629 verificações nativas, 45 do teclado virtual, 72 Java e 6 JNI aprovadas no host com interfaces simuladas.
- [Capacidade](CAPACIDADE.md): medições de fontes, bibliotecas e NDK, com limites conhecidos.
- [Licença](LICENSE): texto GPL fornecido pelo projeto original.

O workflow **Diagnostico de capacidade do runner** é manual e mede CPU, RAM,
espaço livre e ferramentas, sem iniciar um build. Seu relatório fica no Summary
da execução. A margem de espaço configurada na receita é uma precaução de
planejamento, não uma exigência medida deste código.

O repositório contém patch, testes e receita, sem bibliotecas binárias, cache de
compilação ou chaves de assinatura. A receita obtém a fonte completa e as
dependências públicas nas revisões identificadas. Exclui apenas os dados Git LFS
em `tests/**`; arquivos de runtime, assets e código normal são preservados.

O APK unsigned, se o build terminar, precisará ser assinado localmente com uma
chave estável antes de poder ser instalado. Uma assinatura diferente da usada
em uma instalação existente impede a atualização direta. A receita não altera
nenhum dispositivo Android.

O caminho físico consulta `KeyCharacterMap` do dispositivo para obter Unicode
conforme o layout configurado no Android, incluindo Shift, Caps Lock e Num Lock.
O texto acompanha o mesmo evento da tecla, sem uma segunda digitação Java.
Há tratamento de acentos combinados, numpad, pontuação e repetição; texto do IME
atravessa JNI em UTF-8 padrão. AltGr depende de o Android identificar Alt direito
e fornecer o caractere correspondente ao layout.

Limites conhecidos: desconectar um teclado limpa os modificadores físicos de
todos os teclados; Shift travado no painel virtual participa dos atalhos, mas
não recalcula o caractere físico fornecido pelo layout Android. Os testes
locais não substituem compilação completa, instalação ou teste de entrada e
GPU no aparelho. Este projeto não altera o layout configurado pelo usuário.

Fonte de referência: [simfeo/blender](https://github.com/simfeo/blender/tree/android-alpha-3).
Este projeto não é uma distribuição oficial da Blender Foundation.

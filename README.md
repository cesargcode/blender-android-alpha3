# Blender Android Alpha 3: modificadores e digitação física

Patch experimental para `simfeo/blender`, tag `android-alpha-3`, commit
`89479ec817bfe9411ae6ef6f35392d5450e1ad0b`. Corrige a entrega de Shift/Ctrl/Alt/Meta,
a coexistência dos modificadores físicos e virtuais, a limpeza após perda de
foco ou desconexão e a entrega de texto do teclado físico aos campos do Blender.
Mantém a configuração Full e a base gráfica.

**APK Full compilado, inspecionado e assinado em 28/10/2026. O funcionamento no
aparelho ainda não foi testado.** O instalador entregue localmente, separado
do repositório e do draft do GitHub, é
`blender-alpha3-modificadores-full-arm64-signed.apk`, com 287.065.402 bytes e SHA-256:

```text
c7aa70e957ec911a738432d6f8b0b74a9f96cc217a46353222496924bc207ea9
```

A [sexta execução](https://github.com/cesargcode/blender-android-alpha3/actions/runs/40731216675)
concluiu Full, Java/DEX/APK e suas verificações, com envio confirmado dos 16
arquivos. A inspeção local encontrou duas dependências glTF ausentes e acrescentou
`libdraco.so` e `libmeshoptimizer.so` da revisão Android fixada, sem recompilar
ou modificar o conteúdo das entradas originais. O pacote completo passou em
610 verificações estáticas, também aprovadas no APK final assinado; a assinatura v3 e o alinhamento com `zipalign -P 16`
foram verificados. O APK original do GitHub foi preservado e tem hash diferente
do instalador assinado; seu unsigned não está pronto para instalar. A
[receita](build/README.md) registra essa distinção.

- [Patch](blender-alpha3-modificadores.patch): nove arquivos alterados sobre a base fixada.
- [Receita de compilação](build/README.md): Ubuntu 24.04 x64, saída unsigned por padrão.
- [Testes](tests/README.md): 752 verificações locais aprovadas — 629 nativas, 45 do teclado virtual, 72 Java e 6 JNI — com interfaces simuladas.
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

O instalador exige Android 12 ou superior e ARM64. Sua chave é própria: uma
assinatura diferente da instalação existente impede a atualização direta.
Faça backup dos projetos e configurações antes de reinstalar; remover o
aplicativo pode apagar seus dados privados. Novas saídas unsigned da receita
precisam de inspeção e assinatura antes da instalação.

O caminho físico consulta `KeyCharacterMap` do dispositivo para obter Unicode
conforme o layout configurado no Android, incluindo Shift, Caps Lock e Num Lock.
O texto acompanha o mesmo evento da tecla, sem uma segunda digitação Java.
Há tratamento de acentos combinados, numpad, pontuação e repetição; texto do IME
atravessa JNI em UTF-8 padrão. AltGr depende de o Android identificar Alt direito
e fornecer o caractere correspondente ao layout.

Limites conhecidos: desconectar um teclado limpa os modificadores físicos de
todos os teclados; Shift travado no painel virtual participa dos atalhos, mas
não recalcula o caractere físico fornecido pelo layout Android. Os testes
locais e a compilação completa não substituem instalação ou teste de entrada e
GPU no aparelho. Este projeto não altera o layout configurado pelo usuário.

Fonte de referência: [simfeo/blender](https://github.com/simfeo/blender/tree/android-alpha-3).
Este projeto não é uma distribuição oficial da Blender Foundation.

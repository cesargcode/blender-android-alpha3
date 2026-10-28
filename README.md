# Blender Android Alpha 3: correção de modificadores

Patch experimental para `simfeo/blender`, tag `android-alpha-3`, commit
`89479ec817bfe9411ae6ef6f35392d5450e1ad0b`. Corrige a entrega de Shift/Ctrl/Alt/Meta,
a coexistência dos modificadores físicos e virtuais e a limpeza após perda de
foco ou desconexão de teclado. Mantém a configuração Full e a base gráfica.

**Ainda não há APK corrigido compilado ou validado em Android neste projeto.**
Os testes locais verificam funções C++ com interfaces simuladas; não substituem
compilação completa, instalação ou testes de GPU e entrada em um aparelho.

- [Patch](blender-alpha3-modificadores.patch): nove arquivos alterados sobre a base fixada.
- [Receita de compilação](build/README.md): Ubuntu 24.04 x64, saída unsigned por padrão.
- [Testes](tests/README.md): 210 verificações da entrada Android e 45 do teclado virtual aprovadas no host.
- [Capacidade](CAPACIDADE.md): medições de fontes, bibliotecas e NDK, com limites conhecidos.
- [Licença](LICENSE): texto GPL fornecido pelo projeto original.

O workflow **Diagnostico de capacidade do runner** é manual e mede CPU, RAM,
espaço livre e ferramentas, sem iniciar um build. Seu relatório fica no Summary
da execução. Uma compilação precisa de avaliação separada; a presença de 100 GiB
é uma margem de planejamento, não uma exigência medida deste código.

O repositório contém patch, testes e receita, sem bibliotecas binárias, cache de
compilação ou chaves de assinatura. A receita obtém a fonte completa e as
dependências públicas nas revisões identificadas. Exclui apenas os dados Git LFS
em `tests/**`; arquivos de runtime, assets e código normal são preservados.

O APK unsigned, se o build terminar, precisará ser assinado localmente com uma
chave estável antes de poder ser instalado. Uma assinatura diferente da usada
em uma instalação existente impede a atualização direta. A receita não altera
nenhum dispositivo Android.

Limites conhecidos: desconectar um teclado limpa os modificadores físicos de
todos os teclados; entrada Unicode/AltGr não foi reimplementada; comportamento
final também depende dos eventos entregues pelo Android e do driver gráfico.

Fonte de referência: [simfeo/blender](https://github.com/simfeo/blender/tree/android-alpha-3).
Este projeto não é uma distribuição oficial da Blender Foundation.

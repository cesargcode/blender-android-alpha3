# Capacidade para compilar no GitHub Actions

Atualização em 30/10/2026: **novo build Full da revisão Wacom pendente**.
A base continua `89479ec817bfe9411ae6ef6f35392d5450e1ad0b`. A alteração seleciona
CTL-4100 por VID `0x056a`/`0x2d1f` e PID `0x0374`; a ponta será esquerdo,
inferior central e superior direito, preservando S Pen primário direito e mouse.
O APK anterior será mantido; a nova saída assinada prevista é
`blender-alpha3-modificadores-wacom-full-arm64-signed.apk`.

A consulta pública de 30/10 confirmou os três workflows apenas manuais,
zero caches Actions e zero artifacts do run6. Não foi encontrada uma árvore
Android de objetos local reutilizável. A nova correção exige recompilação;
`JOBS=2` no runner público gratuito permanece a escolha já comprovada.
A duração abaixo é histórica, não uma previsão garantida para o novo build.

A suíte nativa passou em **1.088 verificações** (629 anteriores e 459 de movimento),
e passaram **83 Java e 6 JNI**. Com as **45 do teclado virtual mantido**, já
aprovadas, são **1.222 verificações locais**. A preparação final do pacote
reexecuta todas as suítes sobre as fontes entregues. Esses testes não medem capacidade ou duração do build Android.

Avaliação e medições anteriores de 28/10/2026 (Android Alpha 3, Full, sem Wacom):

## Histórico de 28/10: compilação Full e envio concluídos na execução 6

A [execução 6, 40731216675](https://github.com/cesargcode/blender-android-alpha3/actions/runs/40731216675),
no commit `90d8bba59bd2594b64293a943186cfe66aef9935`, terminou com **sucesso** no
runner público `ubuntu-24.04`, com `JOBS=2`, em 28/10/2026. Full, Java/DEX/APK e
as verificações do pacote concluíram; os 16 arquivos tiveram envio confirmado.

| Medição da execução 6 | Valor e alcance |
| --- | --- |
| Job completo | 1 h 35 min 30 s, das 16:58:41 às 18:34:11 UTC |
| Etapa da receita | 1 h 33 min 43 s, incluindo compilação e empacotamento; não é somente Ninja Full |
| Envio para o draft | 53 s, com os 16 arquivos confirmados |
| APK unsigned original baixado | 284.937.749 bytes; SHA-256 `676dc6a0e576dbe1c219946c7ce3323912b6e66ac8c744fe87e8ea9a27161045` |

Após o download, duas dependências glTF ausentes foram acrescentadas localmente
da revisão Android fixada, sem recompilar ou alterar o conteúdo das entradas
originais. O pacote foi inspecionado e assinado. O instalador final, entregue
localmente, tem 287.065.402 bytes e SHA-256
`c7aa70e957ec911a738432d6f8b0b74a9f96cc217a46353222496924bc207ea9`.
O draft contém o unsigned original e não é a entrega desse instalador assinado.
As 610 verificações estáticas, a assinatura v3 e o alinhamento passaram; o
usuário relatou no tablet ponta esquerda, botão inferior direito e superior sem
ação nessa versão; as demais funções não têm validação ampla registrada.
Detalhes no [guia da receita](build/README.md).

O runner gratuito com duas tarefas concluiu três compilações Full. Não houve
necessidade demonstrada de runner pago, cache persistente ou `JOBS=4`. As medições
de RAM e disco disponíveis abaixo são das execuções anteriores; não devem ser
interpretadas como medições de pico da execução 6.

## Histórico: compilação Full nas execuções 4 e 5

O [run 40716244136, execução 5](https://github.com/cesargcode/blender-android-alpha3/actions/runs/40716244136),
compilou o patch com texto físico, no commit `19da3e81136a641ab367173801b5f8fac364d99b`,
em configuração Full no runner público `ubuntu-24.04`, com `JOBS=2`. Concluiu
Java/DEX/APK e as verificações de SDK, ABI ARM64, alinhamento e estado unsigned;
o log exibiu o APK como `272M` e a receita terminou com `BUILD_CHECKS_PASSED`.
A falha seguinte foi de transferência: `HTTP 400: Bad Content-Length` no relatório
vazio `apk-zipalign.txt`. Quatro arquivos auxiliares chegaram ao draft, sem o APK.
Isso confirma capacidade de compilação dessa versão; não comprova funcionamento
no aparelho nem deixa um instalador disponível.

| Medição da execução 5 | Valor e alcance |
| --- | --- |
| Geradores host, etapa cronometrada | 4 min 11,23 s |
| Compilação antecipada dos arquivos alterados e dependências | 7 min 33 s |
| Etapa Ninja Full, após a compilação antecipada | 56 min 09,05 s; não inclui downloads, configuração, host tools, compilação antecipada ou empacotamento |
| Pasta de trabalho após empacotar | 20.015.640.576 bytes = 20,02 GB = 18,64 GiB |
| Espaço livre no filesystem após empacotar | 69.261.111.296 bytes = 69,26 GB = 64,50 GiB |
| RSS máximo registrado por `/usr/bin/time -v` no Ninja Full | 8.124.076 KiB ≈ 7,75 GiB; máximo por processo da árvore, **não pico agregado de todos os processos** |

A [execução 4](https://github.com/cesargcode/blender-android-alpha3/actions/runs/40695500313)
também concluiu Full e empacotamento com duas tarefas, ainda com a versão anterior
à correção de texto. Falhou no leitor de `minSdkVersion`, posteriormente corrigido.
Suas medições históricas foram:

| Medição da execução 4 | Valor e alcance |
| --- | --- |
| Etapa Ninja Full, após os testes de compilação antecipados | 1 h 09 min 14 s; não inclui downloads, configuração, host tools, compilação antecipada ou empacotamento |
| Pasta de trabalho após empacotar | 20.014.944.256 bytes = 20,01 GB = 18,64 GiB |
| Espaço livre no filesystem após empacotar | 69.259.825.152 bytes = 69,26 GB = 64,50 GiB |
| RSS máximo registrado por `/usr/bin/time -v` no Ninja Full | 8.125.244 KiB ≈ 7,75 GiB; máximo por processo da árvore, **não pico agregado de todos os processos** |

Os dados de disco são fotografias ao fim da etapa, não medidas do pico. O pico
agregado de RAM continua sem medição. `JOBS=2` é a configuração comprovada nessas
duas compilações; aumentar para quatro tarefas pode elevar a pressão de memória
e não tem ganho de duração demonstrado. A diferença de duração entre as duas
execuções não isola uma causa. Não há motivo medido para contratar um runner
pago. Sem cache persistente, a próxima VM recomeça downloads e compilação.

A [receita atual](build/README.md) verifica SDK/manifest antes do build, compila
Java e os cinco arquivos C++ alterados antecipadamente e aceita ambos os nomes
do campo mínimo, exigindo minSdk 31 e targetSdk 34. O novo envio confirma o APK
antes dos relatórios e trata arquivos vazios explicitamente. O
[teste real 40730980115](https://github.com/cesargcode/blender-android-alpha3/actions/runs/40730980115)
confirmou estado, tamanho e SHA-256 de cinco arquivos de texto em 29 segundos;
isso testa o mecanismo com textos, não a transferência de um APK.
Na execução 6, esse mesmo mecanismo confirmou o envio do APK e dos relatórios,
conforme registrado acima.
Se uma checagem falhar após existir um APK, o workflow tenta preservá-lo no draft
como `UNVALIDATED`, com relatórios e checksums; isso não transforma a falha em sucesso.

## Estudo anterior e diagnóstico inicial

As seções seguintes preservam o inventário e a avaliação feitos **antes** da
compilação completa. `capacidade-medida.json` continua sendo o registro histórico
desse inventário e do diagnóstico inicial, sem incluir as medições das execuções 4 e 5.

**O diagnóstico inicial do runner público confirmou 85,736 GiB livres, sem limpeza.** Naquele momento, havia folga de disco para tentar a compilação Full, mas ainda não existiam medições do build. Os 100 GiB da receita são uma margem de planejamento configurável, não um requisito medido do Blender.

## Tamanho que foi medido

Foram consultadas as árvores Git das revisões exatas, lidos os pequenos ponteiros Git LFS e somados seus campos `size`. As 30 páginas de árvores das bibliotecas totalizam 28.905 entradas, sem omissões após a paginação. Os tamanhos e hashes SHA-1 dos blobs Git de ponteiros foram conferidos contra seus conteúdos; a fonte foi inspecionada com fetch Git filtrado a blobs menores que 200 bytes. Não foram baixadas as bibliotecas. Para o NDK, foram lidos somente o final e o diretório central do ZIP por HTTP Range (1.826.343 bytes de metadados), somando os tamanhos descompactados de 9.960 entradas.

| Componente | Checkout/instalação, bytes | Cache LFS adicional, bytes | Total lógico, GiB |
| --- | ---: | ---: | ---: |
| Bibliotecas Android ARM64 | 1.190.159.436 | 1.027.217.721 | 2,065 |
| Bibliotecas Linux x64 | 2.406.076.878 | 2.087.273.694 | 4,185 |
| Fonte Blender, sem baixar LFS de `tests/**` | 329.876.578 | 26.547.239 | 0,332 |
| NDK r30 beta1 instalado | 2.259.809.837 | — | 2,105 |
| **Subtotal conhecido** | | | **8,686 GiB = 9.326.961.383 bytes** |

“Cache LFS” representa uma cópia de cada objeto único no armazenamento local de cada repositório, além dos arquivos materializados no checkout. O modelo considera o comportamento habitual do Git LFS, sem deduplicação especial do filesystem. A soma é um orçamento lógico, não um `du` do runner: não inclui arredondamento por blocos, diretórios e objetos Git comprimidos.

O download ZIP do NDK ocupa outros **713.550.176 bytes** enquanto ainda estiver presente. Também faltam no subtotal: SDK platform/build-tools/command-line tools, cabeçalhos Vulkan, pacotes adicionais do sistema, os dois builds de ferramentas host, objetos C/C++, bibliotecas finais, staging de APK e cópias/ZIPs temporários do empacotamento. Portanto, esse subtotal é um **limite inferior do espaço de trabalho completo**, não uma estimativa do pico final.

Revisões e evidências:

- [Bibliotecas Android](https://projects.blender.org/blender/lib-android_arm64/src/commit/59001bef2a4983d0f42e3281cf622a5734c1cb02): 11.795 arquivos Git; 531 arquivos LFS, 525 objetos LFS distintos.
- [Bibliotecas Linux](https://projects.blender.org/blender/lib-linux_x64/src/commit/ecbd06cf6d2a4aa6b00a61ffb479fc81b17aba08): 14.304 arquivos Git; 785 arquivos LFS, 774 objetos LFS distintos.
- [Fonte Blender](https://github.com/simfeo/blender/tree/89479ec817bfe9411ae6ef6f35392d5450e1ad0b): 20.544 arquivos Git; 6.781 ponteiros LFS padrão válidos. Sem `tests/**`, permanecem 365 arquivos LFS necessários ou potencialmente usados fora dos testes.
- [NDK Linux r30 beta1](https://dl.google.com/android/repository/android-ndk-r30-beta1-linux.zip): pacote `ndk;30.0.14904198`, identificado no [catálogo oficial do Android](https://dl.google.com/android/repository/repository2-3.xml).

Os números detalhados e o método estão em `capacidade-medida.json`. Os valores somam arquivos lógicos, mantendo arquivos duplicados no checkout e deduplicando apenas os objetos do cache LFS de cada repositório.

## Economia confirmada ao excluir dados de testes

Os 6.416 arquivos LFS em `tests/**` somam 803.656.629 bytes materializados. A fonte inteira, com cache LFS, custaria 1.955.239.047 bytes. Excluindo somente esse conteúdo LFS, ela custa 356.423.817 bytes: **economia de 1.598.815.230 bytes, cerca de 1,489 GiB**.

Isso preserva o código dos testes e todos os arquivos de runtime de `assets/`, `release/`, `scripts/` e demais diretórios. É melhor aplicar a exclusão específica `lfs.fetchexclude=tests/**` do que restringir arbitrariamente todos os downloads a algumas pastas. O [verificador do Git LFS respeita `lfs.fetchexclude`](https://github.com/git-lfs/git-lfs/blob/main/docs/man/git-lfs-fsck.adoc); uma verificação adicional de ponteiros não materializados também precisa ignorar essa exclusão intencional.

## Diagnóstico observado no GitHub

O [run 40677345178](https://github.com/cesargcode/blender-android-alpha3/actions/runs/40677345178) concluiu o diagnóstico com sucesso. As métricas abaixo foram registradas em **2026-10-28T11:06:29Z**, correspondente a 08:06:29 de Brasília. Nenhuma limpeza foi necessária para obter esse espaço.

| Recurso observado | Valor |
| --- | ---: |
| CPUs | 4, AMD EPYC 7763 |
| RAM total | 16.766.414.848 bytes = 15,615 GiB |
| RAM disponível naquele momento | 15.842.127.872 bytes = 14,754 GiB |
| Swap total | 3.221.221.376 bytes ≈ 3 GiB |
| Disco total do filesystem medido | 154.894.188.544 bytes = 144,256 GiB |
| Disco usado | 62.819.418.112 bytes = 58,505 GiB |
| Disco disponível | **92.057.993.216 bytes = 85,736 GiB** |
| Margem calculada após o subtotal conhecido | **82.731.031.833 bytes = 77,049 GiB** |

A última linha é uma projeção: subtrai os 8,686 GiB inventariados do espaço livre observado. Não representa espaço medido após uma compilação. Ela deixa uma margem substancial para SDK adicional, objetos, link e empacotamento, tornando razoável a primeira tentativa gratuita com paralelismo moderado.

Os **14 GB** da documentação abaixo são a especificação publicada do runner padrão; **85,736 GiB** são a disponibilidade efetiva daquela VM, medida pelo diagnóstico. Não se deve prometer que todas as próximas VMs terão o mesmo espaço: cada job precisa repetir a verificação. O diagnóstico bem-sucedido também não comprova que o build cabe em seis horas ou na RAM disponível.

## Comparação dos runners

| Runner Linux x64 | CPU | RAM | SSD publicado | Avaliação para este build |
| --- | ---: | ---: | ---: | --- |
| Padrão, repositório público, `ubuntu-24.04` | 4 | 16 GB | 14 GB | Escolha para a primeira tentativa: o diagnóstico real encontrou 85,736 GiB livres. |
| Padrão, repositório privado, `ubuntu-24.04` | 2 | 8 GB | 14 GB | Mais restrito em RAM/CPU; não assumir os recursos do runner público. |
| Larger runner Ubuntu x64 | 4 | 16 GB | 150 GB | Resolve a folga de disco; mantém RAM/CPU do padrão público. |
| Larger runner Ubuntu x64 | 8 | 32 GB | 300 GB | Mais folga de disco, memória e paralelismo; ainda depende de build real. |

Os recursos são os documentados pelo [GitHub para runners padrão](https://docs.github.com/en/actions/reference/runners/github-hosted-runners) e [larger runners](https://docs.github.com/en/actions/reference/runners/larger-runners). O runner maior é pago, requer uma organização GitHub Team ou Enterprise Cloud e configuração prévia; uma conta pessoal Pro, isoladamente, não o habilita. Não foi contratado ou iniciado runner pago.

Todos os runners hospedados pelo GitHub têm limite de **6 horas por job**. Na avaliação inicial ainda não existia medição de duração deste build; as execuções 4 e 5 posteriormente concluíram a compilação com duas tarefas Ninja no runner público de quatro CPUs. Isso não mede o efeito de aumentar o paralelismo. Jobs separados precisam transportar resultados e dependências entre máquinas. [Limites de Actions](https://docs.github.com/en/actions/reference/limits).

O diagnóstico inicial justificou tentar o runner público gratuito e conservar os logs; as compilações das execuções 4 e 5 confirmaram essa escolha. Não houve limpeza de arquivos do computador local ou da VM nesta avaliação.

## Decisão prática para a primeira execução

1. Fixar `ubuntu-24.04` x64. A [imagem publicada](https://github.com/actions/runner-images/blob/main/images/ubuntu/Ubuntu2404-Readme.md) inclui JDK 17, CMake, Ninja, Git LFS e compiladores compatíveis; mesmo assim, conferir versões no job e instalar a revisão exata do NDK exigida pela base.
2. Registrar `df -h`, `free -h`, CPU e versões antes de baixar dados, depois das dependências, depois de cada build e após empacotar. A capacidade real disponível naquela VM deve orientar a decisão, sem supor que a etiqueta conceda espaço extra.
3. Excluir somente os dados LFS dos testes. Começar com paralelismo baixo, por exemplo `JOBS=2`, e publicar logs em caso de falha. Isso reduz pressão de RAM, mas não garante duração inferior a seis horas.
4. Tratar espaço insuficiente, encerramento por memória e timeout como resultados mensuráveis do primeiro build. Não exigir automaticamente 100 GiB livres no runner padrão, nem concluir antecipadamente que 14 GB bastam.
5. Se a execução demonstrar falta de recursos, decidir entre otimização/divisão de etapas, um runner Linux próprio ou capacidade maior paga, com autorização específica para custos.

O build de APK usa CPU; uma GPU no runner não é necessária para compilar. A validação gráfica e dos atalhos continua dependendo do dispositivo Android e de seu driver Vulkan. Nenhum dado de capacidade desta análise comprova funcionamento do APK no aparelho.

# Capacidade para compilar no GitHub Actions

Avaliação em 28/10/2026 da base `89479ec817bfe9411ae6ef6f35392d5450e1ad0b` (Android Alpha 3, Full).

**O diagnóstico real do runner público confirmou 85,736 GiB livres, sem limpeza. Há folga de disco para tentar a compilação Full no runner gratuito.** Ainda não há medição do pico de RAM, disco ou tempo do build completo para garantir sua conclusão. Os 100 GiB da receita anterior são uma margem de planejamento configurável, não um requisito medido do Blender.

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

Todos os runners hospedados pelo GitHub têm limite de **6 horas por job**. Não existe medição deste build para afirmar que 4 ou 8 CPUs o concluirão nesse prazo. Jobs separados podem reduzir o risco de timeout por etapa, mas precisam transportar os resultados e dependências entre máquinas; isso não elimina os custos de armazenamento e transferência. [Limites de Actions](https://docs.github.com/en/actions/reference/limits).

O diagnóstico observado não justifica contratar um runner maior neste momento. A próxima etapa é medir o build no runner público gratuito, conservando os logs. Não houve limpeza de arquivos do computador local ou da VM nesta avaliação.

## Decisão prática para a primeira execução

1. Fixar `ubuntu-24.04` x64. A [imagem publicada](https://github.com/actions/runner-images/blob/main/images/ubuntu/Ubuntu2404-Readme.md) inclui JDK 17, CMake, Ninja, Git LFS e compiladores compatíveis; mesmo assim, conferir versões no job e instalar a revisão exata do NDK exigida pela base.
2. Registrar `df -h`, `free -h`, CPU e versões antes de baixar dados, depois das dependências, depois de cada build e após empacotar. A capacidade real disponível naquela VM deve orientar a decisão, sem supor que a etiqueta conceda espaço extra.
3. Excluir somente os dados LFS dos testes. Começar com paralelismo baixo, por exemplo `JOBS=2`, e publicar logs em caso de falha. Isso reduz pressão de RAM, mas não garante duração inferior a seis horas.
4. Tratar espaço insuficiente, encerramento por memória e timeout como resultados mensuráveis do primeiro build. Não exigir automaticamente 100 GiB livres no runner padrão, nem concluir antecipadamente que 14 GB bastam.
5. Se a execução demonstrar falta de recursos, decidir entre otimização/divisão de etapas, um runner Linux próprio ou capacidade maior paga, com autorização específica para custos.

O build de APK usa CPU; uma GPU no runner não é necessária para compilar. A validação gráfica e dos atalhos continua dependendo do dispositivo Android e de seu driver Vulkan. Nenhum dado de capacidade desta análise comprova funcionamento do APK no aparelho.

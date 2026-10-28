# Receita Full ARM64 para Ubuntu 24.04

A receita compila sobre a base `89479ec817bfe9411ae6ef6f35392d5450e1ad0b`, aplica o
patch deste repositório e preserva os recursos Full. Ainda não foi executada de
ponta a ponta. O padrão `SIGN_APK=0` produz um APK unsigned, sem carregar ou gerar
chaves de assinatura. Esse arquivo não pode ser instalado antes da assinatura.

Use Linux x86_64 e uma pasta de trabalho exclusiva, sem espaços. Pré-requisitos:

```bash
sudo apt update
sudo apt install -y build-essential clang-18 cmake ninja-build patchelf \
  zip unzip git git-lfs openjdk-17-jdk python3 python3-pip ca-certificates time
```

O CMake deve ser 3.26 ou superior. Instale os Android SDK Command-Line Tools
oficiais e configure os caminhos:

```bash
export JAVA_HOME=/usr/lib/jvm/java-17-openjdk-amd64
export ANDROID_HOME="$HOME/android-sdk"
export PATH="$JAVA_HOME/bin:$ANDROID_HOME/cmdline-tools/latest/bin:$PATH"
sdkmanager --sdk_root="$ANDROID_HOME" --install --channel=3 \
  'ndk;30.0.14904198' 'build-tools;35.0.1' 'platforms;android-35' 'platform-tools'
```

Leia e aceite as licenças solicitadas pelo SDK. O identificador do pacote NDK é
`30.0.14904198`, sem `-beta1`; corresponde à r30 beta1 usada nas bibliotecas desta
base. Não substitua por outra revisão sem validar a compatibilidade da libc++.
Compile SDK 35, targetSdk 34 e minSdk 31 são valores distintos.

Para iniciar o build unsigned:

```bash
SIGN_APK=0 JOBS=2 MIN_FREE_GIB=40 \
  bash build/build-full-ubuntu24.sh "$HOME/blender-alpha3-build"
```

`MIN_FREE_GIB` é uma margem configurável para interromper cedo a execução; 40 GiB
é o piso escolhido para esta tentativa, e o padrão conservador do script ainda
é 100 GiB. Nenhum deles é o consumo medido. O subtotal lógico conhecido é 8,686 GiB, sem os objetos
de compilação, staging e partes do SDK. Consulte [CAPACIDADE.md](../CAPACIDADE.md)
e a medição do runner para escolher uma margem. O pico real de disco/RAM e a
duração permanecem desconhecidos. O padrão usa duas tarefas; `JOBS=4` pode aumentar
a pressão de memória. Não há promessa de conclusão no limite de seis horas do
GitHub Actions.

O [diagnóstico do runner público](https://github.com/cesargcode/blender-android-alpha3/actions/runs/40677345178)
mediu 4 CPUs, 16.766.414.848 bytes de RAM e 92.057.993.216 bytes livres no workspace
(85,74 GiB). Essa execução permite tentar o build sem limpar ferramentas
pré-instaladas. Outra execução receberá uma VM nova e deve conferir o espaço
novamente. O NDK r30 beta1 exato ainda precisa ser instalado.

A receita registra espaço e memória nas etapas principais e usa `/usr/bin/time`
quando disponível. Mantém arquivos locais para retomada. Não configura cache
remoto ou publica artefatos. Uma nova execução do runner hospedado começa sem os
builds anteriores. O empacotador upstream recria somente seu staging de APK.

Git LFS: só o repositório da fonte usa `lfs.fetchexclude=tests/**`. Download,
`fsck` e verificação de ponteiros não materializados aplicam a mesma exclusão.
As bibliotecas Android/Linux são baixadas e verificadas integralmente. Código
normal em `tests/`, assets, scripts e recursos de runtime não são excluídos.

`prepare-unsigned-package.py` verifica o SHA-256 do `package.sh` original,
preserva sua montagem de bibliotecas, recursos e Java e gera um script temporário
fora do checkout. Substitui somente a localização do script temporário e o trecho
final de assinatura por alinhamento. Não modifica o patch ou a fonte empacotada.
Depois, a receita confere alinhamento, badging, ABI ARM64 e minSdk 31.

Saídas esperadas após sucesso:

```text
blender-alpha3-build/output/blender-alpha3-modificadores-full-arm64-unsigned.apk
blender-alpha3-build/output/SHA256SUMS.txt
blender-alpha3-build/output/build-manifest.txt
blender-alpha3-build/output/apk-badging.txt
blender-alpha3-build/output/apk-signature.txt
blender-alpha3-build/logs/build-full.log
```

Assine o unsigned localmente, com uma chave estável privada. Por exemplo, usando
as Android Build Tools 35.0.1; `apksigner` solicitará a senha sem colocá-la no comando:

```bash
apksigner sign --ks /caminho/privado/chave.p12 --ks-key-alias blender-alpha3 \
  --out blender-alpha3-assinado.apk blender-alpha3-modificadores-full-arm64-unsigned.apk
apksigner verify --verbose --print-certs blender-alpha3-assinado.apk
zipalign -c -p 4 blender-alpha3-assinado.apk
```

Guarde chave e senha fora deste repositório. A assinatura deve ser seguida de
teste funcional em Android ARM64 12 ou superior. Para usar a assinatura integrada
somente em uma máquina local, defina `SIGN_APK=1`, `BLENDER_ANDROID_KEYSTORE` e
`BLENDER_ANDROID_KEY_ALIAS`; a receita pede a senha antes de registrar o log.
`BLENDER_ANDROID_KEYSTORE_PASS`, quando usado, é o **nome** de uma variável que
contém a senha, conforme o contrato do empacotador upstream.

Revisões fixadas:

| Componente | Revisão |
| --- | --- |
| Fonte Blender | `89479ec817bfe9411ae6ef6f35392d5450e1ad0b` |
| Bibliotecas Android | `59001bef2a4983d0f42e3281cf622a5734c1cb02` |
| Bibliotecas Linux | `ecbd06cf6d2a4aa6b00a61ffb479fc81b17aba08` |
| Vulkan-Headers | `b5c8f996196ba4aa6d8f97e52b5d3b6e70f7e4e2` |
| NDK / Build Tools | `30.0.14904198` / `35.0.1` |

Fontes: [guia da base](https://github.com/simfeo/blender/blob/android-alpha-3/build_files/android/BUILDING.md),
[sdkmanager](https://developer.android.com/tools/sdkmanager),
[exclusões no verificador LFS](https://github.com/git-lfs/git-lfs/blob/main/docs/man/git-lfs-fsck.adoc).

#!/usr/bin/env bash
set -euo pipefail
project_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
cd "$project_dir"
for tool in java jar keytool aapt2 apksigner curl unzip zip perl; do
    if ! command -v "$tool" >/dev/null; then
        echo "Missing $tool; sudo apt install openjdk-21-jdk-headless aapt apksigner curl unzip zip" >&2
        exit 127
    fi
done
mkdir -p build/tooling build/classes build/dex build/artifacts
fetch() {
    if [[ ! -s "build/tooling/$2" ]]; then
        curl --fail --location --retry 3 --connect-timeout 20 "$1" -o "build/tooling/$2.part"
        mv "build/tooling/$2.part" "build/tooling/$2"
    fi
}
maven="https://repo.maven.apache.org/maven2"
fetch https://dl.google.com/android/repository/platform-35_r02.zip platform.zip
fetch https://dl.google.com/android/repository/platform-33-ext3_r03.zip platform-resources.zip
fetch https://dl.google.com/android/repository/build-tools_r35_linux.zip build-tools.zip
for artifact in kotlin-compiler-embeddable kotlin-stdlib kotlin-script-runtime kotlin-daemon-embeddable; do
    fetch "$maven/org/jetbrains/kotlin/$artifact/2.0.21/$artifact-2.0.21.jar" "$artifact-2.0.21.jar"
done
fetch "$maven/org/jetbrains/kotlin/kotlin-reflect/1.6.10/kotlin-reflect-1.6.10.jar" kotlin-reflect.jar
fetch "$maven/org/jetbrains/intellij/deps/trove4j/1.0.20200330/trove4j-1.0.20200330.jar" trove4j.jar
fetch "$maven/org/jetbrains/kotlinx/kotlinx-coroutines-core-jvm/1.8.0/kotlinx-coroutines-core-jvm-1.8.0.jar" coroutines.jar
fetch "$maven/org/jetbrains/annotations/13.0/annotations-13.0.jar" annotations.jar
unzip -oq build/tooling/platform.zip -d build/tooling/platform
unzip -oq build/tooling/platform-resources.zip -d build/tooling/resources
unzip -oq build/tooling/build-tools.zip -d build/tooling/android-tools
android_jar="build/tooling/platform/android-35/android.jar"
d8_jar="build/tooling/android-tools/android-15/lib/d8.jar"
if [[ ! -f "$android_jar" || ! -f "$d8_jar" ]]; then
    echo "Unexpected Android SDK archive layout" >&2
    exit 1
fi
rm -rf build/classes build/dex
mkdir -p build/classes build/dex
compiler_cp="build/tooling/kotlin-compiler-embeddable-2.0.21.jar:build/tooling/kotlin-stdlib-2.0.21.jar:build/tooling/kotlin-script-runtime-2.0.21.jar:build/tooling/kotlin-daemon-embeddable-2.0.21.jar:build/tooling/kotlin-reflect.jar:build/tooling/trove4j.jar:build/tooling/coroutines.jar:build/tooling/annotations.jar"
java -cp "$compiler_cp" org.jetbrains.kotlin.cli.jvm.K2JVMCompiler \
    -no-stdlib -no-reflect -jvm-target 1.8 \
    -classpath "$android_jar:build/tooling/kotlin-stdlib-2.0.21.jar:build/tooling/annotations.jar" \
    -d build/classes app/src/main/java/com/raspberry/sppc/MainActivity.kt
jar cf build/app-classes.jar -C build/classes .
java -cp "$d8_jar" com.android.tools.r8.D8 --min-api 26 --lib "$android_jar" \
    --output build/dex build/app-classes.jar build/tooling/kotlin-stdlib-2.0.21.jar build/tooling/annotations.jar
perl -pe 's/<manifest /<manifest package="com.raspberry.sppc" /' app/src/main/AndroidManifest.xml > build/AndroidManifest.xml
aapt2 link -o build/unsigned.apk --manifest build/AndroidManifest.xml -I build/tooling/resources/android-13/android.jar
(cd build/dex && zip -q -u ../unsigned.apk classes.dex)
if [[ ! -f build/debug.keystore ]]; then
    keytool -genkeypair -keystore build/debug.keystore -storepass android -keypass android \
        -alias androiddebugkey -dname 'CN=Android Debug,O=Android,C=US' -keyalg RSA -keysize 2048 -validity 10000
fi
apksigner sign --ks build/debug.keystore --ks-pass pass:android --key-pass pass:android \
    --out build/artifacts/sppc-debug.apk build/unsigned.apk
apksigner verify --verbose --print-certs build/artifacts/sppc-debug.apk
aapt2 dump badging build/artifacts/sppc-debug.apk
(cd build/artifacts && sha256sum sppc-debug.apk > SHA256SUMS)
sha256sum build/artifacts/sppc-debug.apk

# voxelize1

## 1. 概要

このプログラムは、OpenGL のフレームバッファオブジェクト (FBO) とピクセルバッファオブジェクト (PBO)、および平行投影のクリッピングプレーンを利用して、3次元ポリゴンモデルの断面形状を順次スライス取得し、ボクセルデータへ変換（ボクセル化: solid voxelization）する手法を学ぶためのサンプルプログラムです。本プログラムは、以下のブログ記事の解説に対応しています。

- [とっても簡単なボクセル化](https://tokoik.github.io/blog/資料/2009/10/07/texture.html)

`voxelize0` では CPU 側メインメモリへの転送に `glReadPixels()` を直接使用していましたが、本プログラムでは PBO (`GL_PIXEL_PACK_BUFFER`) を経由して GPU メモリ間で非同期にバッファ転送を行うアプローチを実装しています。実行後は生成したボクセルデータを 5 秒周期で順次断面スライス表示します。

## 2. 対応環境

- **Windows**: Visual Studio 2019 以降 / CMake 3.22 以降
- **macOS**: Xcode 12 以降 / CMake 3.22 以降
- **Linux (Ubuntu 等)**: GCC / Clang / CMake 3.22 以降

## 3. ビルド手順

### 3.1 Windows (Visual Studio)

```powershell
cmake -B build
cmake --build build --config Release
```

### 3.2 Linux (Ubuntu)

必要なパッケージをインストールした上でビルドします。

```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake libgl1-mesa-dev libglu1-mesa-dev freeglut3-dev libglew-dev
cmake -B build
cmake --build build
```

### 3.3 macOS (Xcode)

```bash
cmake -B build -G Xcode
cmake --build build --config Release
```

## 4. 起動方法

ビルド完了後、生成された実行ファイルを実行します。

```powershell
./build/Release/voxelize1.exe
```

## 5. 操作方法

- 起動すると、初期化時に 256x256x256 のボクセル化処理が行われます。
- ボクセル化完了後、時間経過に伴って Z 軸方向の断面スライス（256x256 ピクセル）が順次アニメーション表示されます。
- ウィンドウを閉じるかコンソールで `Ctrl+C` を押すと終了します。

## 6. プログラムの解説

### 6.1 PBO の初期化と確保

256x256x256 バイトの領域を持つピクセルバッファオブジェクト (PBO) を作成します。

```cpp
/* ピクセルバッファオブジェクトを作成して結合する */
glGenBuffers(1, &pb);
glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pb);

/* ピクセルバッファオブジェクトのメモリを確保する */
glBufferData(GL_PIXEL_UNPACK_BUFFER, SLICEX * SLICEY * SLICEZ, 0, GL_DYNAMIC_COPY);
glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
```

### 6.2 PBO へのフレームバッファ転送

`GL_PIXEL_PACK_BUFFER` に PBO をバインドした状態で `glReadPixels()` を呼び出すと、CPU メモリではなく PBO のオフセット位置へ直接ピクセルデータが転送されます。

```cpp
/* ピクセルバッファオブジェクトを結合する */
glBindBuffer(GL_PIXEL_PACK_BUFFER, pb);

/* フレームバッファオブジェクトからデータを PBO へ読み出す */
glReadPixels(0, 0, sx, sy, GL_RED, GL_UNSIGNED_BYTE, (GLubyte *)0 + sx * sy * z);
```

### 6.3 PBO からの描画

描画時にも `GL_PIXEL_UNPACK_BUFFER` に PBO をバインドし、`glDrawPixels()` のポインタ引数にバッファ内オフセットを指定することで高速にスライスを描画します。

```cpp
glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pb);
glDrawPixels(SLICEX, SLICEY, GL_LUMINANCE, GL_UNSIGNED_BYTE, (GLubyte *)0 + SLICEX * SLICEY * z);
glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
```

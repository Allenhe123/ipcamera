#guide
1. sudo apt install libtheora-dev libvorbis-dev
2. download ffmpeg-4.3, unzip, cd ffmpeg-4.3
3.  ./configure --prefix=/usr/local/ffmpeg --enable-shared --enable-libfdk-aac --enable-gpl --enable-nonfree --enable-postproc --enable-avfilter --enable-pthreads --enable-libmp3lame --enable-libtheora --enable-libvorbis --enable-libx264 --enable-libxvid --enable-decoder=h264 --enable-encoder=libx264
会报错。

4. 报错：yasm/nasm not found or too old. Use --disable-yasm for a crippled build:
wget http://www.tortall.net/projects/yasm/releases/yasm-1.3.0.tar.gz
tar -zxvf  yasm-1.3.0.tar.gz
./configure  && make  && make install

5. 报错：ERROR: libfdk_aac not found:

https://sourceforge.net/projects/opencore-amr/files/fdk-aac/
下载fdk-aac-0.1.5.tar.gz 
./configure&& make && make install

6. 报错：ERROR: libtheora not found:

http://downloads.xiph.org/releases/theora/?C=M;O=D
libtheora-1.2.0alpha1.tar.gz
./configure && make && make install

7. *** Could not run Ogg test program, checking why...
*** The test program failed to compile or link. See the file config.log for the
*** exact error that occured. This usually means Ogg was incorrectly installed
*** or that you have moved Ogg since it was installed:

http://downloads.xiph.org/releases/ogg/?C=M;O=D
libogg-1.3.2.tar.gz
./configure && make && make install

8. 报错：ERROR: libvorbis not found:
http://downloads.xiph.org/releases/vorbis/libvorbis-1.3.3.tar

9. 报错：ERROR: libmp3lame >= 3.98.3 not found:
http://downloads.sourceforge.net/lame
我下到的是lame-3.99.5.tar.gz
./configure && make && make install

10. 报错：ERROR: libx264 not found:
https://www.videolan.org/developers/x264.html 下载一个稳定版本
./configure --enable-shared  --disable-asm && make && make install

11. 报错：ERROR: libxvid not found:
wget http://downloads.xvid.org/downloads/xvidcore-1.3.5.tar.gz
cd build/generic/
./configure && make && make install

12. WARNING: Option --enable-decoder=libx264 did not match anything
WARNING: using libfdk without pkg-config
WARNING: using libx264 without pkg-config:


export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig:$PKG_CONFIG_PATH
sudo ldconfig

13. 然后cd ffmpeg-4.3 && ./configure ...... && make -j8 && sudo make install

14. 如果系统没有openssl库，也需要安装openssl库




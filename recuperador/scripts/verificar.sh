#!/bin/bash

echo 
echo
wget http://www.cic.unb.br/
echo
echo
wget http://lovelace.aker.com.br/Arquivos/openwrt-ramips-mt7620-zbt-we826-squashfs-sysupgrade.bin
echo
echo
wget http://lovelace.aker.com.br/ISOs/CentOS-6.3-x86_64-minimal.iso
echo
echo
wget http://tvcultura.com.br/upload/tvcultura/programas/programa-imagem-som.jpg
echo
echo
echo
echo "Verificacao: "
ls -lh
echo
echo
echo "HTML"
md5sum cic.html
md5sum index.html
echo
echo
echo "BIN"
md5sum arquivoBin.bin
md5sum openwrt-ramips-mt7620-zbt-we826-squashfs-sysupgrade.bin
echo
echo
echo "ISO"
md5sum arquivoIso.iso
md5sum CentOS-6.3-x86_64-minimal.iso
echo
echo
echo "JPG"
md5sum imagem.jpg
md5sum programa-imagem-som.jpg
echo
echo



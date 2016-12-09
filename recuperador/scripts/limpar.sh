#!/bin/bash
cd ..
echo
echo "Removendo arquivos criados pelo script.sh"
echo
echo

echo "Remover HTML"
rm -rf cic.html
rm -rf index.html
echo

echo "Remover bin"
rm -rf arquivoBin.bin
rm -rf openwrt-ramips-mt7620-zbt-we826-squashfs-sysupgrade.bin
echo 

echo "Remover ISO"
rm -rf arquivoIso.iso
rm -rf CentOS-6.3-x86_64-minimal.iso
echo

echo "Remover jpg"
rm -rf imagem.jpg
rm -rf programa-imagem-som.jpg
echo

echo "Remover recuperadorWeb"
rm -rf recuperadorWeb
echo

echo "Script finalizado!"
echo

#!/bin/bash

make
echo
echo

#OBS.1: ./recuperadorWeb <dominio sem http> <caminho/do/arquivo>
#OBS.2: 'E necess'ario criar os diret'orios do caminho do arquivo a ser baixado (comando mkdir -p <path>)

#Executa o recuperadorWeb para uma pagina html
echo "HTML"
echo
./recuperadorWeb www.cic.unb.br cic.html T

#Executa o recuperadorWeb para um arquivo bin
echo "bin"
echo
mkdir -p Arquivos
./recuperadorWeb lovelace.aker.com.br Arquivos/openwrt-ramips-mt7620-zbt-we826-squashfs-sysupgrade.bin T
echo "Arquivo criado na pasta Arquivos"
echo

#Executa o recuperadorWeb para um arquivo iso
echo "ISO"
echo
mkdir -p ISOs
./recuperadorWeb lovelace.aker.com.br ISOs/Fedora-15-i686-Live-Desktop.iso T
echo "Arquivo criado na pasta ISOs"
echo

#Executa o recuperadorWeb para um arquivo jpg
echo "jpg"
echo
mkdir -p upload/tvcultura/programas
./recuperadorWeb tvcultura.com.br upload/tvcultura/programas/programa-imagem-som.jpg T
echo "Arquivo criado na pasta upload/tvcultura/programas"
echo
echo
echo "Script finalizado!"
echo "Para remover os arquivos criado execute o script limpar.sh"

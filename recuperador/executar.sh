#!/bin/bash

make
echo
echo

#OBS.1: ./recuperadorWeb <dominio sem http> <caminho/do/arquivo>

#Executa o recuperadorWeb para uma pagina html
echo "HTML"
echo
./recuperadorWeb www.cic.unb.br cic.html T
echo "Arquivo HTML criado"
echo

#Executa o recuperadorWeb para um arquivo bin
echo "bin"
echo
./recuperadorWeb lovelace.aker.com.br/Arquivos/openwrt-ramips-mt7620-zbt-we826-squashfs-sysupgrade.bin arquivoBin.bin T
echo "Arquivo BIN criado"
echo

#Executa o recuperadorWeb para um arquivo iso
echo "ISO"
echo
./recuperadorWeb lovelace.aker.com.br/ISOs/CentOS-6.3-x86_64-minimal.iso arquivoIso.iso T
echo "Arquivo ISO criado"
echo

#Executa o recuperadorWeb para um arquivo jpg
echo "jpg"
echo
./recuperadorWeb tvcultura.com.br/upload/tvcultura/programas/programa-imagem-som.jpg imagem.jpg T
echo "Arquivo JPG criado"
echo
echo
echo "Script finalizado!"
echo "Para verificar os arquivos execute o script verificar.sh"
echo "Para remover os arquivos criado execute o script limpar.sh"

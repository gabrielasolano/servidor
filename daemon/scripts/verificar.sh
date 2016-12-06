#!/bin/bash

# Esse script 'e apenas para facilitar a visualizacao dos testes que eu fiz.
# Ele nao deve ser executado.

cd ..
echo
echo

#Recupera uma pagina HTML (em outro terminal)
md5sum pagina.html diretorio/buildbot-waterfall.html
echo
md5sum arquivoBin.bin diretorio/Arquivos/openwrt-ramips-mt7620-zbt-we826-squashfs-sysupgrade.bin
echo
md5sum arquivoIso.iso diretorio/ISOs/CentOS-6.3-x86_64-minimal.iso
echo
md5sum arquivoPdf.pdf diretorio/Arquivos/ProjetoNovoDPIeIPS.pdf
echo
md5sum imagem.jpg diretorio/upload/tvcultura/programas/programa-imagem-som.jpg
echo

echo "Script finalizado!"
echo "Para remover os arquivos criado execute o script limpar.sh"

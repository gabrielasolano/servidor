#!/bin/bash

echo "Verificando arquivos de resultado-get"
md5sum resultado-get/*
echo "Apagando arquivos de resultado-get"
rm resultado-get/*

echo "Verificando arquivos de resultado-put"
md5sum resultado-put/*
echo "Apagando arquivos de resultado-put"
rm resultado-put/*

echo "Fim do script"

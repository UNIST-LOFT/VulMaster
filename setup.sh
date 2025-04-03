#!/bin/bash
set -e

# Install dependencies
conda create -n vulmaster python=3.9
conda activate vulmaster
pip install -r requirements.txt
conda deactivate

# Install CodeT5
pip install gdown
mkdir bugfix_pretrain_with_ast
cd bugfix_pretrain_with_ast
gdown 1057u16sqSf14w51CA0fZt-WJ6FjS2X6I
cd ..

# Unzip compressed files
cd vulfix_data
gunzip train.json.gz
cd ..
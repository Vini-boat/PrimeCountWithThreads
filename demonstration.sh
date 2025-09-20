echo  "Compilar o programa."
./build.sh
echo "Executar com k = 1."
./primos 1
echo "Executar com k = 4."
./primos 4
echo "Executar com k = nº de CPUs disponíveis (mostrar este valor)."
echo "Número de CPUs disponíveis: $(nproc)"
./primos 0
echo "Desafio: Benchmark Automático"
./primos -b
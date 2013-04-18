if [ $# -ne 1 ]; then
  echo "Usage: `basename $0` extension"
  exit 1
fi

fgrep "FAIL" out.csv | sed 's/FAIL//' > results-failed$1.csv
sed '/FAIL/ d' out.csv | sed '/bitstream/ d' > results-nofail$1.csv
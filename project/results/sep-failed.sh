fgrep "FAIL" out.csv | sed 's/FAIL//' > results-failed.csv
sed '/FAIL/ d' out.csv | sed '/bitstream/ d' > results-nofail.csv
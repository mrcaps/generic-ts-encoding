PATH=
PY=C:/python33/python.exe
IN=G:/data/timeseries/timeseriesdata
OUT=G:/tmp

all: test

run:
	$(PY) convertmany.py $(IN) $(OUT)

combine:
	$(PY) combinedb.py $(OUT)

ingest:
	$(PY) ingest.py

test:
	$(PY) convert.py
#	$(PY) convertmany.py

toobig:
	$(PY) report_too_big.py $(IN)/_TOO_BIG res/too_big.json $(OUT)/toobig.csv

play:
	$(PY) report_too_big.py -h
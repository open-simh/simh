;JSR ADRBRK
dep 000042 264000000105
;CONO CONO APR,200000+PIA
dep 000100 700200200001
;CONO PI,2200+<200_-PIA>
dep 000101 700600002300
;MOVE A,100
dep 000102 200040000100
;MOVEM A,200
dep 000103 202040000200
;JRST 400
dep 000104 254000000400
;0
dep 000105 000000000000
;CONO APR,40000+PIA
dep 000106 700200040001
;JRST 12,@ADRBRK
dep 000107 254520000105
;JRST LOOP
dep 000400 254000000102

;Check address stop: data fetch from 100.
dep acond 12
dep as 100
go 100
if (PC != 000103) echof "FAIL: stop dfetch 100"; ex pc; exit 1

;Check address stop: write to 200.
dep acond 06
dep as 200
continue
if (PC != 000104) echof "FAIL: stop write 200"; ex pc; exit 1

;Check address stop: instruction fetch from 400.
dep acond 22
dep as 400
continue
if (PC != 000102) echof "FAIL: stop ifetch 400"; ex pc; exit 1

;Check address break: data fetch from 100.
break 106
dep acond 11
dep as 100
continue
if (PC != 000106) echof "FAIL: break dfetch 100"; ex pc; exit 1

;Check address break: write to 200.
dep acond 05
dep as 200
continue
if (PC != 000106) echof "FAIL: break write 200"; ex pc; exit 1

;Check address break: instruction fetch from 400.
dep acond 21
dep as 400
continue
if (PC != 000106) echof "FAIL: break ifetch 400"; ex pc; exit 1

echof "PASS"
exit 0

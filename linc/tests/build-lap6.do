# This tests the emulator by rebuilding all of LAP6 from manuscripts.
# The output is a new tape with the rebuilt LAP6 and an empty index.
# The blocks 300-337 should compare identical.

cd %~p0

# LAP6 saves its state to tape, so use a copy.
copy lap6-manuscripts.linc clobbered.linc
copy empty.linc lap6.linc

# The bootstrap LAP6 must be on tape unit 0, and the default is to
# have the result going to tape unit 1 blocks 300-337.  It's also
# possible to change these parameters to write the result to another
# block range, or to the same tape unit.  To run LAP6 from another
# tape unit or from another block range, it's necessary to change the
# symbols 7E and 7D in the manuscripts.
set env u0=0
set env u1=1
set env b=3

set crt disabled
set dpy text
attach tape%u0% clobbered.linc
if "%u0%" != "%u1%" attach tape%u1% lap6.linc

expect "1"
boot tape0

# This scripts the build process as per the instructions in
# "LAP6 Manuscript Listings", May 1, 1967.
call am "MSDISPLY"
call cv
call cp 10 blocks 340 00
call am "ADDMS LO"
call cv
call cp 1 block 343 24
call restart
call am "FILECOMS"
call cv
call cp 4 blocks 340 20
call restart
call am "COPYFILE"
call cv
call cp 3 blocks 340 14
call restart
call am "COPY"
call cv
call cp 2 blocks 340 36
call restart
call am "CONVERT"
call cv
call cp 3 blocks 340 30
call restart
call am "CV DIS"
call cv
call cp 2 blocks 340 10
call restart
call am "DISX PX"
call am "TELETYPE"
call cv
call cp 2 blocks 340 25
call restart
call am "PRINT MS"
call am "TELETYPE"
call cv
call cp 1 block 342 22
call restart
call am "LIST"
call am "TELETYPE"
call cv
call cp 1 block 341 33
call cp 1 block 342 27
call cp 1 block 343 35
call cp 2 blocks 346 12
call cp 1 block 346 17

echo "LAP6 REBUILD PASSED"
quit

:am
echo *** Add manuscript %1 ***
send "\eAM "
noexpect
expect "-/AM"
continue
send "%1,0"
noexpect
expect "%1,0"
continue
send "\r"
noexpect
expect "_"
continue
return

:cv
echo *** Convert ***
send "\eCV"
noexpect
expect "-/CV"
continue
send "\r"
noexpect
# PRINT MS has an undefined symbol 5T.
expect "ERRORS" send "\r"; continue
expect "MEMORY ALLOCATION"
continue
send "\r"
noexpect
expect "SYMBOLS" send "\r"; continue
expect "_"
continue
return

:cp
echo *** Copy %1 %2 from %3 to %b%%4 ***
send "\eCP"
noexpect
expect "-/CP"
continue
send "\r"
noexpect
expect "TO BLOCK"
continue
send "%1\r"
noexpect
expect "%1 BLOCKS"
continue
send "%3\r"
noexpect
expect "FROM BLOCK %3"
continue
send "%u0%\r"
noexpect
expect "UNIT %u0%,"
continue
send "%b%%4\r"
noexpect
expect "TO BLOCK %b%%4"
continue
send "%u1%"
noexpect
expect "UNIT %u1%"
continue
send "\r"
noexpect
expect "_"
continue
return

:restart
noexpect
expect "1"
go 20
return

CC= cl

LUA_A=	liblua.a
CORE_O=	lapi.o lcode.o ldebug.o ldo.o ldump.o lfunc.o lgc.o llex.o lmem.o \
	lobject.o lopcodes.o lparser.o lstate.o lstring.o ltable.o ltm.o  \
	lundump.o lvm.o lzio.o
LIB_O=	lauxlib.o lbaselib.o ldblib.o liolib.o lmathlib.o loslib.o ltablib.o \
	lstrlib.o loadlib.o linit.o print.o

ALL_O= $(CORE_O) $(LIB_O) $(LUA_O) $(LUAC_O)
ALL_T= $(LUA_A) $(LUA_T) $(LUAC_T)
ALL_A= $(LUA_A)


CFLAGS= /DLUA_BUILD_AS_DLL 

#%.o:%c
#	echo $@

report_server.exe: liblua.a report_server.o 
	cl /Fereport_server report_server.o  liblua.a 

report_server.o : report_server.c
	cl /c /DWIN_32 report_server.c /Foreport_server.o

liblua.a:	$(ALL_O)
	lib /OUT:liblua.a $(ALL_O)

clean:
	del report_server.exe report_server.o liblua.a $(ALL_O)
##
#client & test

###script output here.

lapi.o : lapi.c
	cl /c lapi.c /Folapi.o
lauxlib.o : lauxlib.c
	cl /c lauxlib.c /Folauxlib.o
lbaselib.o : lbaselib.c
	cl /c lbaselib.c /Folbaselib.o
lcode.o : lcode.c
	cl /c lcode.c /Folcode.o
ldblib.o : ldblib.c
	cl /c ldblib.c /Foldblib.o
ldebug.o : ldebug.c
	cl /c ldebug.c /Foldebug.o
ldo.o : ldo.c
	cl /c ldo.c /Foldo.o
ldump.o : ldump.c
	cl /c ldump.c /Foldump.o
lfunc.o : lfunc.c
	cl /c lfunc.c /Folfunc.o
lgc.o : lgc.c
	cl /c lgc.c /Folgc.o
linit.o : linit.c
	cl /c linit.c /Folinit.o
liolib.o : liolib.c
	cl /c liolib.c /Foliolib.o
llex.o : llex.c
	cl /c llex.c /Follex.o
lmathlib.o : lmathlib.c
	cl /c lmathlib.c /Folmathlib.o
lmem.o : lmem.c
	cl /c lmem.c /Folmem.o
loadlib.o : loadlib.c
	cl /c loadlib.c /Foloadlib.o
lobject.o : lobject.c
	cl /c lobject.c /Folobject.o
lopcodes.o : lopcodes.c
	cl /c lopcodes.c /Folopcodes.o
loslib.o : loslib.c
	cl /c loslib.c /Foloslib.o
lparser.o : lparser.c
	cl /c lparser.c /Folparser.o
lstate.o : lstate.c
	cl /c lstate.c /Folstate.o
lstring.o : lstring.c
	cl /c lstring.c /Folstring.o
lstrlib.o : lstrlib.c
	cl /c lstrlib.c /Folstrlib.o
ltable.o : ltable.c
	cl /c ltable.c /Foltable.o
ltablib.o : ltablib.c
	cl /c ltablib.c /Foltablib.o
ltm.o : ltm.c
	cl /c ltm.c /Foltm.o
lua.o : lua.c
	cl /c lua.c /Folua.o
luac.o : luac.c
	cl /c luac.c /Foluac.o
lundump.o : lundump.c
	cl /c lundump.c /Folundump.o
lvm.o : lvm.c
	cl /c lvm.c /Folvm.o
lzio.o : lzio.c
	cl /c lzio.c /Folzio.o
print.o : print.c
	cl /c print.c /Foprint.o


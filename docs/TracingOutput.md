# Tracing file

The tracing output is simplified list of called functions.
The own functions are displayed with parameter and values, the external function just by function call text.
In case of error, the file, line, call time and error message are traced

## Tracing line prefix

The most tracing lines are prefixed with a hint.

--> funcion start  
<-- function end  
V  
E  
I - input parameter  
?  
!  
N - unformatted value (NYI)  
O - output parameter  
C - call  
R - return  
X - exception  

## Tracing output

### 


### Error appeared

```
      --> X4xy::assignInstance
        I tItemRevision = 0 (tag_t) <AAAAAAAAAAAAAA>
*** C:\Development\workspace\X4xy_BMIDE\src\server\X4xy\x4xy_structure_loader.cxx line : 50, at 11:10:05
*** WSOM_ask_name2 ( tItemRevision, &value )
*** Error 7005: The specified enquiry can not be executed
        R Value = 7005 
      <-- X4xy::assignInstance (0.00 sec) 
```

# Syslog file

All errors are automatically tracked in syslog file, independent of configuration

```
 TCTYPE_ask_name2 ( tOjectType, &typeName ) returns [39021]
  Teamcenter ERROR: 39021 [Fehler bei Suche nach entsprechendem ImanType  aufgetreten. ]
  in file [.\src\server\X4xy\x4xy_structure_loader.cxx], line [77], at 06:00:12
```

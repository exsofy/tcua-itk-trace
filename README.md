# Teamcenter®  ITK tracing and journalling C++ support

The repository contains code for simple implementation of basic tracing 
and/or full Teamcenter® journalling.

It uses C macros and C++ scope functionality to make easy to use for
programmer and provide full journalling support for issue analysis.

## Functionality
- full support of Teamcenter journalling
- simplified tracing for code run overview
- itk call return code evaluation
- proven in single/multiple shared libraries, handlers, stadalone executables and SOA services

The tracing and journalling are switched out per default and might be
activated on customer site for offline analysis.

## Usage
The code has to be extended by macros for each function, ITK call and 
optionally function return.

```C++
extern "C" DLLAPI int libACME_register_callbacks () {
  XFY_TFCE_P0 ();
 
  XFY_TCALL ( CUSTOM_register_exit ( "libACME", "USER_gs_shell_init_module", (CUSTOM_EXIT_ftn_t)ACME_gs_shell_init_module) );

  XFY_TRET_OK;
}
```

## Activation
- Journalling is activated by standard Teamcenter process
- Tracing is activated by environment variables
```
set XFY_TRACE_FILE=C:\temp\*P_*D.log
set XFY_TRACE_FLAGS=255
```

## Results
In journalling (jnl file)
...
```
@*23758.5.2.2.55       CUSTOM_register_exit ( 'libtcpb',  'USERSERVICE_register_methods',  0000000180005A40)
@*23758.5.2.2.55       CUSTOM_register_exit returns 0 [in 0.000000 secs]
@*23758.5.2.2.56       --> 0.860000s
@*23758.5.2.2.56       libACME_register_callbacks ()
@*23758.5.2.2.56.1         --> 0.860000s
@*23758.5.2.2.56.1         CUSTOM_register_exit ( 'libACME',  'USER_gs_shell_init_module',  00007FFA9B7D1096)
@*23758.5.2.2.56.1         CUSTOM_register_exit returns 0 [in 0.000000 secs]
@*23758.5.2.2.56       libACME_register_callbacks returns 0 [in 0.000000 secs]
@*23758.5.2.2      CUSTOM_register_callbacks returns 0 [in 0.219000 secs, 2 DB trips]
```
...
```
@*24019.3.2.6.13.5.18.205          ACME_rh_check_sibling_status ()
@*24019.3.2.6.13.5.18.205.1            --> 0.968000s
@*24019.3.2.6.13.5.18.205.1            EPM_ask_root_task ( 0000e3f2)
...
@*24019.3.2.6.13.5.18.205.6            ACME_ItemRevisionCheckSiblings ( 0000d0e9,  0,  '  none  , TCM Released, Other ',  NULL)
@*24019.3.2.6.13.5.18.205.6.1              --> 0.968000s
@*24019.3.2.6.13.5.18.205.6.1              WSOM_where_referenced ( 0000d0e9,  1,  n_referencers,  levels,  referencers)
...
@*24019.3.2.6.13.5.18.205.6.8              ACME_ItemRevisionCheckSiblings ( 0000d0e9,  00002ab3,  0,  '  none  , TCM Released, Other ',  NULL)
@*24019.3.2.6.13.5.18.205.6.8.1                --> 1.000000s
@*24019.3.2.6.13.5.18.205.6.8.1                BOM_create_window ( window)
...
@*24019.3.2.6.13.5.18.205.6.8.11               BOM_line_ask_attribute_tag returns 0 [in 0.000000 secs], value = 0000e839 <QRAAAwiKI$MupD>
@*24019.3.2.6.13.5.18.205.6.8              ACME_ItemRevisionCheckSiblings returns 0 [in 1.360000 secs, 173 DB trips]
@*24019.3.2.6.13.5.18.205.6            ACME_ItemRevisionCheckSiblings returns 0 [in 1.392000 secs, 183 DB trips], handlerDecision = 0000004600000000
@*24019.3.2.6.13.5.18.205          ACME_rh_check_sibling_status returns 0 [in 1.392000 secs, 184 DB trips]
```
...

## Extension
For automatic ITK memory release on function break use the [c++ memory 
module](https://github.com/exsofy/tcua-itk-memory)

# Common tracing pattern

## Function tracing

### Minimal function
Minimal function without parameter or error handling, which is returning ITK_OK
```C++
int DataDefinition::load() {
	XFY_TFCE_P0();
	
	XFY_TRET_OK;
}
```

## Universal function tracing
### Entry point
### Parameters

## Entry point with parameters together


## General error handling

All the macros have basic erro handling build in. Each error will be 
reported in syslog immediately, regardless if it is removed from error
stack later.

Try to ommit the error by pre-check, do not trace the function or 
mask its return with & 0.

#### Masked return value
```C++
 XFY_TREP ( AOM_refresh ( tObj, false ) & 0 );
```

### Function calls without resource release
If no memory is allocated or the memory release is performed by scope object,
the base error handling is use the *XFY_TREP* macro to get the return value.

### With resource release
From the point where resource is reserved (e.g. Memory allocation, BOM Window opening)
the resource management has to be introduced

#### Resources managed by scope object
This is the simpliest way. There is no difference to code without resource 
management.

#### Resource managed by release block
The macro *XFY_TCALL_L* jumps in case of error to defined label.

#### Resource managed by return code check
If labels are not acceptable, use the *XFY_TREP* macro to get the return value
and check it by code.


## Special error handling

### ITK function call wrapper
If an singe ITK function is wrapped with less additional functionality
```C++
int XFY_POM_is_object_a ( tag_t tObject, tag_t tClassID, logical *isA )
{
  XFY_TFCE_P3 ( tObject, I, tClassID, I, isA, O );

  tag_t tObjectClass;
  XFY_TCALL ( POM_class_of_instance ( tObject, &tObjectClass ) );

  XFY_TRET ( POM_is_descendant ( tClassID, tObjectClass, isA ) );
}
```

### Single function call error handling

If a single function call is evaluated, use *XFY_TREP* to get the error code.
Return the error value by *XFY_TERR*, if necessary.
```C++
	XFY::ITKString docID;
	int iFail = XFY_TREP( AOM_ask_value_string( tItem, docIdAttrName, &docID ) );

	if ( iFail == ITK_ok ) {
		...
	} else if ( iFail == PROP_not_found ) {
		...
	} else {
		XFY_TERR(iFail);
	}
```


### Multiple function call error handling

If mutliple function shall be handled the same way is usage with label possible
The macro *XFY_TCALL_L* jumps to defined label, which can manage the error.
The return value is accessible over *XFY_JNZ_VALUE* macro.

```C++
	XFY::ITKString uifValue;
	PROP_value_type_t propType;
	XFY::ITKString propTypeName; // scope variable with destructor must be defined before first label use

	XFY_TCALL_L( AOM_UIF_ask_value ( tRequestedObject, propertyName, &uifValue, PROPERROR ) );

	XFY_TCALL_L ( AOM_ask_value_type( tRequestedObject, propertyName, &propType, &propTypeName, PROPERROR ));

	switch ( propType ) {
		case PROP_date : {
			date_t tcDate;
			XFY_TCALL_L ( AOM_ask_value_date( tRequestedObject, propertyName, &tcDate ), PROPERROR );
			val.dateValue = tcDate;
		} break;
		case PROP_string : {
			XFY::ITKString value;
			XFY_TCALL_L ( AOM_ask_value_string( tRequestedObject, propertyName, &value ), PROPERROR );
		} break;
		case PROP_double :
			XFY_TCALL_L ( AOM_ask_value_double( tRequestedObject, propertyName, &val.doubleValue ), PROPERROR );
			break;
		case PROP_int :
			XFY_TCALL_L ( AOM_ask_value_int( tRequestedObject, propertyName, &val.intValue ), PROPERROR );
			break;
		case PROP_logical :
			XFY_TCALL_L ( AOM_ask_value_logical( tRequestedObject, propertyName, &val.boolValue ), PROPERROR );
			break;
	}
	goto PROPOK;
PROPERROR:		
	if ( XFY_JNZ_VALUE == PROP_not_found || XFY_JNZ_VALUE == other_error_code  ) {
		val.stringValue = "No such property";					
	} else {
		XFY_TERR(iFail);
	}
PROPOK:	
```

### Continue after error

If the error can be ignored, but is reported
The macro *XFY_TCALL_L* jumps to defined label, which manages error without break the run.
The return value is accessible over *XFY_JNZ_VALUE* macro.

```C++
	for ( auto itPropValue = propertyValues.begin(); itPropValue != propertyValues.end(); itPropValue++  ) {
		// apply remaining property value
		bool isConsumed = false;
		if ( itPropValue->onObj == PropertyValue::ITEM ) {
			XFY_CALL_L ( acme_setProperty( tItemType, tNewItem, itPropValue->name.c_str(), itPropValue->value, &isItemChanged ), PROPERTYUNSET );
			isItemChanged |= isConsumed;
		} else if ( itPropValue->onObj == PropertyValue::REVISION ) {
			XFY_CALL_L ( acme_setProperty( tItemRevType, tNewRev, itPropValue->name.c_str(), itPropValue->value, &isConsumed ), PROPERTYUNSET );
			isRevisionChanged |= isConsumed;
		}
PROPERTYUNSET:		
		if ( XFY_JNZ_VALUE != ITK_ok ) {
			runtimeInfo.getStoredError(tNewRev,EMH_severity_error,XFY_JNZ_VALUE);
		}
	}
```

## Convert standard journalling

### Headers
To use the XFY functions, replace journal/journal.h with xfy/trace/xfy_trace_itk.h

### Standard journalling
```C
#include <journal/journal.h>

int AE_ask_tool_input_formats(tag_t   tool,
							  int*    format_count,
							  char*** input_formats)
{
	int retcode = ITK_ok;
	if( my_journal_switch_is_on() )
	{
		JOURNAL_routine_start ("AE_ask_tool_input_formats");
		JOURNAL_tag_in (tool);
		JOURNAL_output_argument ("format_count");
		JOURNAL_output_argument ("input_formats");
		JOURNAL_routine_call ();
		JOURNAL_comment ("You can add comments to the journal file if you want");
	}

	if( my_journal_switch_is_on() )
	{
		JOURNAL_integer_out ("format_count", *format_count);
		JOURNAL_string_array_out ("input_formats", *format_count, *input_formats);
		JOURNAL_return_value (retcode);
		JOURNAL_routine_end();
	}

	return retcode;
}
```

### XFY journalling

```C++
#include <xfy/trace/xfy_trace_itk.h>

int AE_ask_tool_input_formats(tag_t   tool,
							  int*    format_count,
							  char*** input_formats)
{
	XFY_TFCE_P3 ( tool, I, format_count, O, input_formats, O );

	XFY_TMSG ( "You can add comments to the journal file if you want");

	XFY_TRET_OK;
}
```


function __sIEve_initializeHooks(jsHook)
{
	window.attachEvent('onbeforeunload',function ()
	{
		jsHook.rescanForElements(window.document);
		window.detachEvent('onbeforeunload',arguments.callee);
	});

	window.attachEvent('onunload',function ()
	{
		try {
			jsHook.unloadWindow(window.document);
			if ( window.__sIEve_rescanForElementsTimer )
			{
				window.clearTimeout(window.__sIEve_rescanForElementsTimer);
				window.__sIEve_rescanForElementsTimer = null;
				window.navigator.__sIEve_rescanForElementsIntervalHandler = null;
			}
			window.__sIEve_logElement = null;
			window.document.createElement = null;
			window.detachEvent('onunload',arguments.callee);
		}
		catch (err) { /* Ignore errors on close of sIEve */ }
	});

	window.document.attachEvent('onstop',function ()
	{
		try
		{
			jsHook.unloadWindow(window.document);
			if ( window.__sIEve_rescanForElementsTimer )
			{
				window.clearTimeout(window.__sIEve_rescanForElementsTimer);
				window.__sIEve_rescanForElementsTimer = null;
				window.navigator.__sIEve_rescanForElementsIntervalHandler = null;
			}
			window.__sIEve_logElement = null;
			window.document.createElement = null;
			window.document.detachEvent('onstop',arguments.callee);
		}
		catch (err) { /* Ignore errors on close of sIEve */ }
	});

	if ( window.document.location.href.indexOf('blank.htm') < 0 )
	{
		window.navigator.__sIEve_rescanForElementsInterval = 50;
	}
	if ( ! window.navigator.__sIEve_rescanForElementsIntervalHandler )
	{
		window.navigator.__sIEve_rescanForElementsIntervalHandler = function()
		{
			jsHook.rescanForElements();
			window.__sIEve_setRescanForElementsTimeout();
		};
		window.__sIEve_setRescanForElementsTimeout();
	}
	else
	{
		if ( !window.library ) window.setTimeout(function(){jsHook.rescanForElements(window.document);},100);
	}

	window.__sIEve_logElement = function(elem)
	{
		jsHook.logElement(elem);
		var child = elem.firstChild;
		while ( child ) {
			__sIEve_logElement(child);
			child = child.nextSibling;
		}
	};
	
	window.__sIEve_logDetectedCycle = function(elem)
	{
		jsHook.logDetectedCycle(elem);
	};
	
	var nativeCreateElement = window.document.createElement;
	window.document.createElement = function(tag) {
		var elem = nativeCreateElement(tag);
		__sIEve_logElement(elem);
		return elem;
	};
}

function __sIEve_setRescanForElementsTimeout()
{
	window.navigator.__sIEve_rescanForElementsInterval *= 2;
	if ( window.navigator.__sIEve_rescanForElementsInterval >= 5000 )
	{
		window.navigator.__sIEve_rescanForElementsInterval = 5000;
	}
	window.__sIEve_rescanForElementsTimer = window.setTimeout(window.navigator.__sIEve_rescanForElementsIntervalHandler,window.navigator.__sIEve_rescanForElementsInterval);
}

function __sIEve_overloadCloneNode(elem)
{
	if (elem.nodeType == 1 && !elem.__sIEve_nativeCloneNode)
	{
		try
		{
			elem.__sIEve_nativeCloneNode = elem.cloneNode;
			elem.cloneNode = __sIEve_customCloneNode;
		}
		catch ( err ) { /* Some elements doesn't allow override e.g. <EMBED> */ }
	}
}

function __sIEve_customCloneNode(deep)
{	
	var self = this;
	self.removeAttribute("cloneNode"); // Remove the override (to avoid problems with clones of clones)
	var clone = self.cloneNode(deep);  // Call the original native method
	self.cloneNode = arguments.callee; // Restore the override	
	if ( clone )
	{
		window.__sIEve_logElement(clone);
	}
	return clone;
}

//Filling array with default property names which don't need to be scanned
{
	var ___sIEve_defaultProperties = new Array();
	var __dummyElement = window.document.createElement("div");
	for ( var i in __dummyElement ) ___sIEve_defaultProperties[i] = true;
	___sIEve_defaultProperties["cloneNode"] = true;
	___sIEve_defaultProperties["___sIEve_refs"] = true;
	___sIEve_defaultProperties["__sIEve_nativeCloneNode"] = true;
}

window.navigator.___sIEveCrossRefScanId = 0;
var ___sIEve_markedObjects = null;

function ___sIEve_crossRefScan(object)
{
	___sIEve_markedObjects = new Array();
	___sIEve_crossRefScanObject(object,object,++window.navigator.___sIEveCrossRefScanId,"");
	for ( var i = 0; i < ___sIEve_markedObjects.length; i++)
	{
		// Delete the scanned mark for either a HTMLDom object or jsobject; Try both types; one will fail.
		try { 	delete ___sIEve_markedObjects[i].__sIEveScannedMark } catch (err) {}
		try {	___sIEve_markedObjects[i].removeAttribute("__sIEveScannedMark"); } catch (err) {} 
	}
	___sIEve_markedObjects = null;
}

function ___sIEve_identifyObject(object)
{
	var idstr = "";
	if ( object && typeof(object.nodeType) != "undefined" && typeof(object.className) != "undefined" )  // To be sure target is HTMLElement
	{
		idstr = "<"+object.nodeName + (object.id=="" ? "" : (" id='"+object.id+"'"))+ ">";
	}
	else
	{
		idstr = "["+dltypeof(object)+"]";
	}
	return idstr;
}

function ___sIEve_crossRefScanObject(sourceObject, object, crossRefScanId, referencePath)
{
	if ( ___sIEve_isValidTarget(object) )
	{
		if ( typeof(object.__sIEveScannedMark) != "undefined" && object.__sIEveScannedMark == crossRefScanId)
		{
			if ( sourceObject == object )
			{
				sourceObject["_CIRCULAR REFERENCE: "+ referencePath] = "path = " + referencePath;
				__sIEve_logDetectedCycle(sourceObject);
			}
		}
		else		
		{
			try
			{
				object.__sIEveScannedMark = crossRefScanId;
				___sIEve_markedObjects[___sIEve_markedObjects.length] = object;
				if ( ___sIEve_isCollection(object) )
				{
					for ( var i=0; i<object.length; i++) 
					{
						var targetObject = object[i];
						var newReferencePath = referencePath + "." + i;
						___sIEve_crossRefScanObjectRegistration(sourceObject,targetObject,newReferencePath);
						if ( ___sIEve_isValidTarget(targetObject) ) ___sIEve_crossRefScanObject(sourceObject,targetObject,crossRefScanId,newReferencePath);
					}
				}
				else
				{
					for ( var i in object )
					{
						if ( ! ___sIEve_defaultProperties[i] )
						{
							var targetObject = object[i];
							var newReferencePath = referencePath + "." + i;
							___sIEve_crossRefScanObjectRegistration(sourceObject,targetObject,newReferencePath);
							if ( ___sIEve_isValidTarget(targetObject) ) ___sIEve_crossRefScanObject(sourceObject,targetObject,crossRefScanId,newReferencePath);
						}
					}
				}
			}
			catch (err) {}
		}
	}
}

function ___sIEve_crossRefScanObjectRegistration(sourceObject,targetObject, referencePath)
{
	try
	{
		if ( targetObject && typeof(targetObject.nodeType) != "undefined" && typeof(targetObject.className) != "undefined" )  // To be sure target is HTMLElement
		{
			var index = ___sIEve_identifyObject(sourceObject)+referencePath;
			targetObject["_REFERENCE: " + index] = "path = " + index;
		}
	}
	catch ( err ) {}
}

function ___sIEve_isValidTarget(target)
{
	try
	{
		if ( typeof(target.navigator) != "undefined" )
		{
			// Don't scan window objects
			return false;
		}
		if ( typeof(target.nodeType) != "undefined" && target.className == "undefined" )
		{
			// Don't scan XMLNodes
			return false;
		}
		if ( typeof(target.tagName) != "undefined")
		{
			var tagName = target.tagName;
			if ( tagName  == "LINK" ) 
			{
				//alert("LINK");
				return false;  // Don't scan <LINK>
			}
		} 
		var type = typeof(target);
		if ( type == "object" || type == "function" )
		{
			return true;
		}
	}
	catch (err) {}
	return false;
}

function ___sIEve_isCollection(target)
{
	if ( typeof(target.item) != "undefined" )
	{
		return true;
	}
	return false;
}

//========== TEMP CODE FOR DEBUGGING
function dltypeof( vExpression )
{	
	try
	{
		var sTypeOf = typeof vExpression;
		if( sTypeOf == "function" )
		{
			var sFunction = vExpression.toString();
			if( ( /^\/.*\/$/ ).test( sFunction ) )
			{
				return "regexp";
			}
			else if( ( /^\[object.*\]$/i ).test( sFunction ) )
			{
				sTypeOf = "object"
			}
		}
		if( sTypeOf != "object" )
		{
			return sTypeOf;
		}
		
		switch( vExpression )
		{
			case null:
				return "null";
			case window:
				return "window";
			case window.event:	
				return "event";
		}
		
		if( window.event && ( event.type == vExpression.type ) )
		{
			return "event";
		}
		
		var fConstructor = vExpression.constructor;
		if( fConstructor != null )
		{
			switch( fConstructor )
			{																	
				case Array:
					sTypeOf = "array";
					break;
				case Date:
					return "date";
				case RegExp:
					return "regexp";
				case Object:
					sTypeOf = "jsobject";
					break;
				case ReferenceError:
					return "error";
				default:
					var sConstructor = fConstructor.toString();
					var aMatch = sConstructor.match( /\s*function (.*)\(/ );
					if( aMatch != null )
					{
						return aMatch[ 1 ];
					}
				
			}
		}

		var nNodeType = vExpression.nodeType;
		if( nNodeType != null )
		{			
			switch( nNodeType )
			{
				case 1:
					if ( typeof(vExpression.className) == "string" )
					{
						return "htmldomnode";
					}
					else
					{
						return "xmldomnode";
					}
					break;
				case 3:
					if ( typeof(vExpression.specified) != "undefined" )
					{
						return "xmltextnode";
					}
					else
					{
						return "htmltextnode";
					}
			}
		}
		
		if( vExpression.toString != null )
		{
			var sExpression = vExpression.toString();
			var aMatch = sExpression.match( /^\[object (.*)\]$/i );
			if( aMatch != null )	
			{
				var sMatch = aMatch[ 1 ];
				switch( sMatch.toLowerCase() )
				{
					case "event":
						return "event";
					case "math":
						return "math";
					case "error":	
						return "error";
					case "mimetypearray":
						return "mimetypecollection";
					case "pluginarray":
						return "plugincollection";
					case "windowcollection":
						return "window";
					case "nodelist":
					case "htmlcollection":
					case "elementarray":
						return "domcollection";
				}
			}
		}
		
		if( vExpression.moveToBookmark && vExpression.moveToElementText )
		{
			return "textrange";
		}
		else if( vExpression.callee != null )
		{
			return "arguments";
		}
		else if( typeof(vExpression.item) != "undefined" )	
		{
			return "xmldomcollection";
		}
		return sTypeOf;
	}
	catch (err) {}
	return "undefined";
}


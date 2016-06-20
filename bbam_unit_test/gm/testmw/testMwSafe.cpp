#include <memory>
#include <fstream>
#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#define	MW_ERROR(...) do{fprintf(stderr,"%s:%d:\t",__FILE__,__LINE__);fprintf(stderr,__VA_ARGS__);}while(0)
#define	MW_INFO  MW_ERROR
#define	MW_ASSERT(x) assert(x)
#define	MW_MSG MW_ERROR

/*****************************************************************************************
*
*	parse_items_safe to modify the parse items it add the items num to it
*
*	Jul 20th 2012 wang ruichuan create this function to replace parse_items
*****************************************************************************************/
static int parse_items_safe ( char **items, int itemmax, char *buf, int bufsize, int *name_start_pos, int *value_start_pos )
{
    int item = 0;
    int InString = 0, InItem = 0;
    char *p = buf;
    char *bufend = &buf[bufsize];
    char *bufstart = &buf[0];

    *name_start_pos = 0;
    *value_start_pos = 0;
    if ( itemmax < 1 )
    {
        MW_ERROR ( "itemmax is %d < 1\n", itemmax );
        return -2;
    }



    while ( p < bufend )
    {
        switch ( *p )
        {
        case 13:	// CR
            /*we set the content into the string*/
            *p = '\0';
            InString = 0;
            InItem = 0;
            ++p;
            break;

        case '#':	// Found comment
        case '[':	// ccz +:section name start
        case ']':	// ccz +:section name end
            *p = '\0';
            while ( ( *p != '\n' ) && ( p < bufend ) )
            {
                *p = '\0';
                ++p;
            }
            InString = 0;
            InItem = 0;
            break;

        case '\n':
            InItem = 0;
            InString = 0;
            *p++ = '\0';
            break;

        case ' ':
        case '\t':					// Skip whitespace, leave state unchanged
            if ( InString )
            {
                ++p;
            }
            else
            {
                *p++ = '\0';
                InItem = 0;
            }
            break;

        case '"':					// Begin/End of String
            *p++ = '\0';
            if ( !InString )
            {
                if ( ( item % 3 ) == 2 )
                {
                    *value_start_pos = ( int ) ( p - bufstart );
                }
                //	MW_DEBUG("item=%d,pos=[%d,%d]",item,*name_start_pos,*value_start_pos);

                if ( item >= itemmax )
                {
                    MW_ERROR ( "item overflow %d itemmax\n", itemmax );
                    return -2;
                }
                items[item++] = p;
                InItem = ~InItem;
            }
            else
            {
                InItem = 0;
            }
            InString = ~InString;	// Toggle
            break;

        default:
            if ( !InItem )
            {
                if ( ( item % 3 ) == 0 )
                {
                    *name_start_pos = ( int ) ( p - bufstart );
                    //	MW_DEBUG("item=%d,pos=[%d,%d]",item,*name_start_pos,*value_start_pos);
                }
                if ( ( item % 3 ) == 2 )
                {
                    *value_start_pos = ( int ) ( p - bufstart );
                    //	MW_DEBUG("item=%d,pos=[%d,%d]",item,*name_start_pos,*value_start_pos);
                }
                if ( item >= itemmax )
                {
                    MW_ERROR ( "item overflow %d itemmax\n", itemmax );
                    return -2;
                }

                items[item++] = p;
                InItem = ~InItem;
            }
            ++p;
            break;
        }
    }

    if ( ( item % 3 ) == 2 ) // not 3 times
    {
        if ( item >= itemmax )
        {
            MW_ERROR ( "item overflow %d itemmax\n", itemmax );
            return -2;
        }
        items[item++] = 0;
        //	MW_DEBUG("append a item=%d,items=0,pos=[%d,%d]\n",item-1,*name_start_pos,*value_start_pos);
    }

    return ( item - 1 );

}

/*****************************************************************************************
*
*	parse_items2_safe to modify the parse items it add the items num to it
*
*	Jul 20th 2012 wang ruichuan create this function to replace parse_items2
*****************************************************************************************/
static int parse_items2_safe ( char **items, int itemmax, char *buf, int bufsize, int *name_start_pos, int *value_start_pos )
{
    int item = 0;
    int InString = 0, InItem = 0;
    char *p = buf;
    char *bufend = &buf[bufsize];
    char *bufstart = &buf[0];

    *name_start_pos = 0;
    *value_start_pos = 0;
    if ( itemmax < 1 )
    {
        MW_ERROR ( "itemmax is low %d\n", itemmax );
        return -2;
    }

    while ( p < bufend )
    {
        switch ( *p )
        {
        case 13:	// CR
            *p = '\0';
            ++p;
            InString = 0;
            InItem = 0;
            break;

        case '#':	// Found comment
        case '[':	// ccz +:section name start
        case ']':	// ccz +:section name end
            *p = '\0';
            while ( ( *p != '\n' ) && ( p < bufend ) )
            {
                *p = '\0';
                ++p;
            }
            InString = 0;
            InItem = 0;
            break;

        case '\n':
            InItem = 0;
            InString = 0;
            *p++ = '\0';
            break;

        case ' ':
        case '\t':					// Skip whitespace, leave state unchanged
            if ( InString )
            {
                ++p;
            }
            else if ( ( item % 3 ) == 2 )
            {
                if ( item >= itemmax )
                {
                    MW_ERROR ( "itemmax is overflow %d\n", itemmax );
                    return -2;
                }
                *value_start_pos = ( int ) ( p - bufstart );
                items[item++] = p;
                while ( ( *p != '#' ) && ( *p != '\n' ) && ( p < bufend ) )
                {
                    ++p;
                }
                *p++ = '\0';
                InItem = 0;
                //	MW_DEBUG("item[%d]=%s,pos=[%d,%d]",item-1,items[item-1], *name_start_pos,*value_start_pos);
            }
            else
            {
                *p++ = '\0';
                InItem = 0;
            }
            break;

        case '"':					// Begin/End of String
            *p++ = '\0';
            if ( !InString )
            {
                if ( ( item % 3 ) == 2 )
                {
                    *value_start_pos = ( int ) ( p - bufstart );
                }
                if ( item >= itemmax )
                {
                    MW_ERROR ( "itemmax is overflow %d\n", itemmax );
                    return -2;
                }
                items[item++] = p;
                InItem = ~InItem;
                while ( *p != '\"' && p < bufend && *p  != '\n' &&
                        *p != '\r' )
                {
                    p ++;
                }
                if ( *p != '\"' )
                {
                    MW_ERROR ( "\" is not matched\n" );
                    return -1;
                }
				*p = '\0';
				p ++;
                //	MW_DEBUG("item=%d,pos=[%d,%d]",item-1,*name_start_pos,*value_start_pos);
            }
            else
            {
                InItem = 0;
                //	MW_DEBUG("item[%d]=%s,pos=[%d,%d]",item-1,items[item-1], *name_start_pos,*value_start_pos);
            }
            break;

        default:
            if ( !InItem )
            {
                if ( ( item % 3 ) == 0 )
                {
                    if ( item >= itemmax )
                    {
                        MW_ERROR ( "itemmax is overflow %d\n", itemmax );
                        return -2;
                    }
                    *name_start_pos = ( int ) ( p - bufstart );
                    items[item++] = p;
                    InItem = 0;
                    while ( ( *p != ' ' ) && ( *p != '\t' && *p != '\n' && *p != '#' && *p != '\r' ) && ( p < bufend ) )
                    {
                        ++p;
                    }
                    *p = '\0';
                    //	MW_DEBUG("item[%d]:%s,pos:[%d,%d]",item-1,items[item-1], *name_start_pos,*value_start_pos);
                }
                else if ( ( item % 3 ) == 2 )
                {
                    if ( item >= itemmax )
                    {
                        MW_ERROR ( "itemmax is overflow %d\n", itemmax );
                        return -2;
                    }
                    *value_start_pos = ( int ) ( p - bufstart );
                    items[item++] = p;
                    InItem = 0;
                    while ( ( *p != '#' ) && ( *p != '\n' && *p != '\r' ) && ( p < bufend ) )
                    {
                        ++p;
                    }
                    *p = '\0';
                    //	MW_DEBUG("item[%d]:%s,pos:[%d,%d]",item-1,items[item-1], *name_start_pos,*value_start_pos);
                }
                else
                    /*	if (*p == '=')*/
                {

                    if ( item >= itemmax )
                    {
                        MW_ERROR ( "itemmax is overflow %d\n", itemmax );
                        return -2;
                    }
                    items[item++] = p;
                    InItem = 0;
                    ++p;
                    *p = '\0';
                    //	MW_DEBUG("item[%d]:%s,pos:[%d,%d]",item-1,items[item-1], *name_start_pos,*value_start_pos);
                }
            }
            ++p;
            break;
        }
    }

    if ( ( item % 3 ) == 2 ) // not 3 times
    {
        if ( item >= itemmax )
        {
            MW_ERROR ( "itemmax is overflow %d\n", itemmax );
            return -2;
        }
        items[item++] = 0;
        //	MW_DEBUG("append a item=%d,items=0,pos=[%d,%d]\n",item-1,*name_start_pos,*value_start_pos);
    }

    return ( item - 1 );
}


typedef enum
{
    ITEM_SESSION = 1,
    ITEM_NAME	= 2,
    ITEM_VALUE	= 3,
} ITEM_ENUM_t;

typedef struct
{
    int startpos;
    int endpos;
    ITEM_ENUM_t flag;   /* 1 for session name ,2 for id string 3 for value string*/
} ITEM_PARSE_t;


#define	SET_VALUE_POS()\
do\
{\
	if (pSesString)\
	{\
		MW_ASSERT(idstart != -1 && idend != -1);\
		MW_ASSERT(idstart <= idend);\
		if (pIdString || pValString)\
		{\
			MW_ERROR("error of compile [%d]\n",lines);\
		}\
		else\
		{\
			if (item >= itemmax)\
			{\
				MW_ERROR("overflow %d max\n",itemmax);\
				return -2;\
			}\
	\
			pCurItem = &(pItems[item]);\
			item ++;\
			pCurItem->startpos = idstart;\
			pCurItem->endpos = idend;\
			pCurItem->flag =ITEM_SESSION;\
		}\
	}\
	else if (pIdString)\
	{\
		if (pValString)\
		{\
			MW_ASSERT(idstart != -1 && idend != -1);\
			MW_ASSERT(idstart <= idend);\
			MW_ASSERT(valstart != -1 && valend != -1);\
			MW_ASSERT(valstart <= valend);\
			if ((item +1)>= itemmax)\
			{\
				MW_ERROR("overflow %d max\n",itemmax);\
				return -2;\
			}\
			pCurItem = &(pItems[item]);\
			item ++;\
			pCurItem->startpos = idstart;\
			pCurItem->endpos = idend;\
			pCurItem->flag = ITEM_NAME;\
			\
			pCurItem = &(pItems[item]);\
			item ++;\
			pCurItem->startpos = valstart;\
			pCurItem->endpos = valend;\
			pCurItem->flag = ITEM_VALUE;\
		}\
		else\
		{\
			MW_ERROR("Parse [%d] error offset (%d) ,(%p)\n",lines,(pIdString - buf),pValString);\
			if ((item +1)>= itemmax)\
			{\
				MW_ERROR("overflow %d max\n",itemmax);\
				return -2;\
			}\
				/*we want to make the null string*/\
				pCurItem = &(pItems[item]);\
				item ++;\
				pCurItem->startpos = idstart;\
				pCurItem->endpos = idend;\
				pCurItem->flag = ITEM_NAME;\
				\
				pCurItem = &(pItems[item]);\
				item ++;\
				pCurItem->startpos = 0;\
				pCurItem->endpos = 0;\
				pCurItem->flag = ITEM_VALUE;\
		}\
	}\
}while(0)

#define	MAKE_STRINGS_DEFAULT()\
			do\
			{\
				pIdString =NULL;\
				pValString =NULL;\
				pSesString =NULL;\
				idstart = -1;\
				idend = -1;\
				valstart = -1;\
				valend = -1;\
				hasequal = 0;\
			}while(0)

/*****************************************************************************************
*
*	parse_items2_session_safe to modify the parse items it add the items num to it
*
*	Jul 20th 2012 wang ruichuan create this function to replace parse_items
*****************************************************************************************/
int parse_items2_session_safe ( ITEM_PARSE_t* pItems, int itemmax, char *buf, int bufsize )
{
    int item = 0;
    char *p = buf;
    char *bufend = &buf[bufsize];
    char *pSesString = NULL;
    char *pIdString = NULL;
    char *pValString = NULL;
    int idstart = -1, idend = -1, valstart = -1, valend = -1;
    ITEM_PARSE_t *pCurItem = NULL;
    int hasequal = 0;
    int lines = 1;

    if ( itemmax < 1 )
    {
        MW_ERROR ( "itemmax is low %d\n", itemmax );
        return -2;
    }

    while ( p < bufend )
    {
        switch ( *p )
        {
        case '#':	// Found comment
            p ++ ;
            while ( ( *p != '\n' ) && ( *p != '\r' )  && ( p < bufend ) )
            {
                ++p;
            }
            SET_VALUE_POS();
            MAKE_STRINGS_DEFAULT();
            break;
        case '[':	// ccz +:section name start
            p++;
            if ( pSesString )
            {
                MW_MSG ( "parse error [%d]\n", lines );
                while ( ( *p != '\r' ) && ( *p != '\n' ) && ( *p != '#' ) && ( p < bufend ) )
                {
                    p ++ ;
                }
                MAKE_STRINGS_DEFAULT();
                continue;
            }
            pSesString = p;
            while ( ( *p != '\r' ) && ( *p != '\n' ) && ( *p != ']' ) && ( *p != '#' ) && ( p < bufend ) )
            {
                p ++;
            }
            if ( *p != ']' )
            {
                MW_MSG ( "Find [%d] not valid session name\n", ( lines ) );
                while ( ( *p != '\r' ) && ( *p != '\n' ) && ( *p != '#' ) && ( p < bufend ) )
                {
                    p ++ ;
                }
                MAKE_STRINGS_DEFAULT();
                continue;
            }
            idstart = ( pSesString - buf );
            idend = ( p - buf );
            p ++;
            while ( ( *p != '\r' ) && ( *p != '\n' ) && ( *p != '#' ) && ( p < bufend ) )
            {
                p ++ ;
            }
            break;

        case ']':
            MW_MSG ( "meet error [%d]\n", lines );
            while ( ( *p != '\r' ) && ( *p != '\n' ) && ( *p != '#' ) && ( p < bufend ) )
            {
                p ++;
            }
            MAKE_STRINGS_DEFAULT();
            break;
        case '\n':
        case 13:	// CR
            if ( *p == '\n' )
            {
                lines ++;
            }
            p++ ;
            SET_VALUE_POS();
            MAKE_STRINGS_DEFAULT();
            break;

        case ' ':
        case '\t':					// Skip whitespace, leave state unchanged
            while ( ( *p == ' ' ) || ( *p == '\t' ) )
            {
                p ++;
            }
            break;

        case '"':					// Begin/End of String
            if ( pValString || pIdString == NULL )
            {
                MW_MSG ( "parse [%d] error without any id\n", lines );
                while ( ( *p != '\r' ) && ( *p != '\n' ) && ( *p != '#' ) && ( p < bufend ) )
                {
                    p ++;
                }
                MAKE_STRINGS_DEFAULT();
                continue;
            }
            p ++;
            pValString = p;
            while ( ( *p != '\r' ) && ( *p != '\n' ) && ( p < bufend ) && ( *p != '"' ) )
            {
                p ++;
            }
            if ( *p != '"' )
            {
                MW_MSG ( "parse [%d] error in quotation\n", lines );
                while ( ( *p != '\r' ) && ( *p != '\n' ) && ( *p != '#' ) && ( p < bufend ) )
                {
                    p ++;
                }
                MAKE_STRINGS_DEFAULT();
                continue;
            }
            valstart = ( pValString - buf );
            valend = ( p - buf );
            p ++;
            break;
        case '=':
            if ( pValString || pIdString == NULL )
            {
                MW_MSG ( "parse [%d] error without any id\n", lines );
                while ( ( *p != '\r' ) && ( *p != '\n' ) && ( *p != '#' ) && ( p < bufend ) )
                {
                    p ++;
                }
                MAKE_STRINGS_DEFAULT();
                continue;
            }
            p ++;
            hasequal = 1;
            break;
        case 0x0:
            /*this is the end of the buffer*/
            goto out;

        default:
            if ( pIdString == NULL )
            {
                pIdString = p;
                while ( ( *p != '\r' ) && ( *p != '\n' ) && ( *p != ' ' ) && ( *p != '\t' )
                        && ( *p != '=' ) && ( p < bufend ) && ( *p != '#' ) && ( *p != '"' )
                        && ( *p != '[' ) && ( *p != ']' ) )
                {
                    p ++;
                }
                idstart = ( pIdString - buf );
                idend = ( p - buf );
            }
            else if ( pValString == NULL )
            {
                if ( hasequal == 0 )
                {
                    MW_MSG ( "no equal [%d]\n", lines );
                    while ( ( *p != '\r' ) && ( *p != '\n' ) && ( *p != '#' ) && ( p < bufend ) )
                    {
                        p ++;
                    }
                    MAKE_STRINGS_DEFAULT();
                    continue;
                }
                pValString = p;
                while ( ( *p != '\r' ) && ( *p != '\n' ) && ( *p != ' ' ) && ( *p != '\t' )
                        && ( *p != '=' ) && ( p < bufend ) && ( *p != '#' ) && ( *p != '"' )
                        && ( *p != '[' ) && ( *p != ']' ) )
                {
                    p ++;
                }
                valstart = ( pValString - buf );
                valend = ( p - buf );
            }
            else
            {
                //MW_MSG("parse [%d] parse id error\n",lines);
                /* we modify the end value to set it ok*/
                while ( ( *p != '\r' ) && ( *p != '\n' ) && ( *p != ' ' ) && ( *p != '\t' )
                        && ( *p != '=' ) && ( p < bufend ) && ( *p != '#' ) && ( *p != '"' )
                        && ( *p != '[' ) && ( *p != ']' ) )
                {
                    p ++;
                }
                valend = ( p - buf );
                //MAKE_STRINGS_DEFAULT();
                continue;
            }
            break;
        }
    }
out:
    SET_VALUE_POS();
    MAKE_STRINGS_DEFAULT();
    return ( item - 1 );
}

#undef SET_VALUE_POS
#undef	MAKE_STRINGS_DEFAULT


int TestParseItems2Safe ( char *file )
{
    std::ifstream ifs ( file );
    std::filebuf *pbuf = NULL;
    std::auto_ptr<char> pChar2 ( new char[32] );
    std::auto_ptr<char> pParseChar2 ( new char[32] );
    char *pParseChar = NULL;
    char *pChar = NULL;
    char** pItems = NULL;
    int itemmax = 100;
    int size;
    int ret;
    int namestart, valuestart;
    int gotitems;
    int i;
    try
    {
        ifs.exceptions ( ifs.failbit );
        pbuf = ifs.rdbuf();
        size = pbuf->pubseekoff ( 0, std::ios_base::end, std::ios_base::in );
        pbuf->pubseekoff ( 0, std::ios_base::beg, std::ios::in );

        pChar2.reset ( new char[size + 1] );
        pChar = pChar2.get();
        pbuf->sgetn ( pChar, size );
        pChar[size] = '\0';
        ifs.close();

try_again:
        pParseChar2.reset ( new char[size + 1] );
        pParseChar = pParseChar2.get();
        memcpy ( pParseChar, pChar, size + 1 );
        if ( pItems )
        {
            free ( pItems );
        }
        pItems = NULL;
        pItems = ( char** ) malloc ( sizeof ( char* ) *itemmax );
        if ( pItems == NULL )
        {
            goto fail;
        }
        memset ( pItems , 0, sizeof ( *pItems ) *itemmax );

        ret = parse_items2_safe ( pItems, itemmax, pParseChar, size, &namestart, &valuestart );
        if ( ret == -2 )
        {
            itemmax <<= 1;
            goto try_again;
        }
        else if ( ret < 0 )
        {
            goto fail;
        }

        gotitems = ret;
        for ( i = 0; i < gotitems; i += 3 )
        {
            fprintf ( stdout, "%s = \"%s\"\n", pItems[i], pItems[i + 2] );
        }


    }

    catch ( const std::ios_base::failure& e )
    {
        std::cout << "can not open " << file << std::endl;
        ret = -1;
        goto fail;
    }

    return 0;
fail:
    if ( pItems )
    {
        free ( pItems );
    }
    pItems = NULL;

    return ret;
}


int TestParseItemsSafe ( char *file )
{
    std::ifstream ifs ( file );
    std::filebuf *pbuf = NULL;
    std::auto_ptr<char> pChar2 ( new char[32] );
    std::auto_ptr<char> pParseChar2 ( new char[32] );
    char *pParseChar = NULL;
    char *pChar = NULL;
    char** pItems = NULL;
    int itemmax = 100;
    int size;
    int ret;
    int namestart, valuestart;
    int gotitems;
    int i;
    try
    {
        ifs.exceptions ( ifs.failbit );
        pbuf = ifs.rdbuf();
        size = pbuf->pubseekoff ( 0, std::ios_base::end, std::ios_base::in );
        pbuf->pubseekoff ( 0, std::ios_base::beg, std::ios::in );

        pChar2.reset ( new char[size + 1] );
        pChar = pChar2.get();
        pbuf->sgetn ( pChar, size );
        pChar[size] = '\0';
        ifs.close();

try_again:
        pParseChar2.reset ( new char[size + 1] );
        pParseChar = pParseChar2.get();
        memcpy ( pParseChar, pChar, size + 1 );
        if ( pItems )
        {
            free ( pItems );
        }
        pItems = NULL;
        pItems = ( char** ) malloc ( sizeof ( char* ) *itemmax );
        if ( pItems == NULL )
        {
            goto fail;
        }
        memset ( pItems , 0, sizeof ( *pItems ) *itemmax );

        ret = parse_items_safe ( pItems, itemmax, pParseChar, size, &namestart, &valuestart );
        if ( ret == -2 )
        {
            itemmax <<= 1;
            goto try_again;
        }
        else if ( ret < 0 )
        {
            goto fail;
        }

        gotitems = ret;
        for ( i = 0; i < gotitems; i += 3 )
        {
            fprintf ( stdout, "%s = \"%s\"\n", pItems[i], pItems[i + 2] );
        }


    }

    catch ( const std::ios_base::failure& e )
    {
        std::cout << "can not open " << file << std::endl;
        ret = -1;
        goto fail;
    }
    if ( pItems )
    {
        free ( pItems );
    }
    pItems = NULL;
    return 0;
fail:
    if ( pItems )
    {
        free ( pItems );
    }
    pItems = NULL;

    return ret;
}

int TestParseSessionFile ( char* file )
{
    std::ifstream ifs ( file );
    std::filebuf *pbuf = NULL;
    std::auto_ptr<char> pChar2 ( new char[32] );
    std::auto_ptr<char> pParseChar2 ( new char[32] );
    char *pParseChar = NULL;
    char *pChar = NULL;
    ITEM_PARSE_t* pItems = NULL;
    int itemmax = 100;
    int size;
    int ret;
    int gotitems;
    int i, maxsize = 32;


    try
    {
        ifs.exceptions ( ifs.failbit );
        pbuf = ifs.rdbuf();
        size = pbuf->pubseekoff ( 0, std::ios_base::end, std::ios_base::in );
        pbuf->pubseekoff ( 0, std::ios_base::beg, std::ios::in );

        pChar2.reset ( new char[size + 1] );
        pChar = pChar2.get();
        pbuf->sgetn ( pChar, size );
        pChar[size] = '\0';
        ifs.close();

alloc_again:
        if ( pItems )
        {
            free ( pItems );
        }
        pItems = NULL;
        pItems = ( ITEM_PARSE_t* ) malloc ( sizeof ( *pItems ) * itemmax );
        if ( pItems == NULL )
        {
            MW_ERROR ( "can not allocate %d\n", sizeof ( *pItems ) *itemmax );
            ret = -1;
            goto fail;
        }
        ret = parse_items2_session_safe ( pItems, itemmax, pChar, size + 1 );
        if ( ret == -2 )
        {
            itemmax <<= 1;
            goto alloc_again;
        }

        gotitems = ret;
        gotitems ++;

        for ( i = 0; i < gotitems; )
        {
            ITEM_PARSE_t *pCurItem = & ( pItems[i] );
            ITEM_PARSE_t *pValItem = & ( pItems[i + 1] );
            switch ( pCurItem->flag )
            {
            case ITEM_SESSION:
                if ( maxsize <= ( pCurItem->endpos - pCurItem->startpos ) )
                {
                    maxsize = ( pCurItem->endpos - pCurItem->startpos + 1 );
                    pParseChar2.reset ( new char[maxsize] );
                }
                pParseChar = pParseChar2.get();
                pChar = pChar2.get();
                pChar += pCurItem->startpos;
                memset ( pParseChar, 0, maxsize );
                memcpy ( pParseChar, pChar, ( pCurItem->endpos - pCurItem->startpos ) );
                fprintf ( stdout, "[%s]\n", pParseChar );
                i ++;
                break;
            case ITEM_NAME:
                if ( ( i + 1 ) >= gotitems )
                {
                    MW_ERROR ( "exceeded %d items\n", gotitems );
                    ret = -1;
                    goto fail;
                }
                if ( maxsize <= ( pCurItem->endpos - pCurItem->startpos ) )
                {
                    maxsize = ( pCurItem->endpos - pCurItem->startpos + 1 );
                    pParseChar2.reset ( new char[maxsize] );
                }
                pParseChar = pParseChar2.get();
                pChar = pChar2.get();
                pChar += pCurItem->startpos;
                memset ( pParseChar, 0, maxsize );
                //MW_ERROR("start pos %d endpos %d\n",pCurItem->startpos , pCurItem->endpos);
                memcpy ( pParseChar, pChar, ( pCurItem->endpos - pCurItem->startpos ) );
                fprintf ( stdout, "%s = ", pParseChar );
                fflush ( stdout );
                if ( maxsize <= ( pValItem->endpos - pValItem->startpos ) )
                {
                    maxsize = ( pValItem->endpos - pValItem->startpos + 1 );
                    pParseChar2.reset ( new char[maxsize] );
                }
                pParseChar = pParseChar2.get();
                pChar = pChar2.get();
                //MW_ERROR("start pos %d endpos %d\n",pValItem->startpos,pValItem->endpos);
                pChar += pValItem->startpos;
                memset ( pParseChar, 0, maxsize );
                memcpy ( pParseChar, pChar, ( pValItem->endpos - pValItem->startpos ) );
                fprintf ( stdout, "\"%s\"\n", pParseChar );
                i += 2;
                fflush ( stdout );
                break;
            default:
                MW_ERROR ( "unknown flag %d\n", pCurItem->flag );
                goto fail;
                break;
            }
        }

    }

    catch ( const std::ios_base::failure& e )
    {
        std::cout << "can not open " << file << std::endl;
        ret = -1;
        goto fail;
    }

    if ( pItems )
    {
        free ( pItems );
    }
    pItems = NULL;
    return 0;
fail:
    if ( pItems )
    {
        free ( pItems );
    }
    pItems = NULL;
    return ret;
}




int main ( int argc, char*argv[] )
{
    int ret;
    if ( argc < 2 )
    {
        fprintf ( stderr, "%s filename\n", argv[0] );
        exit ( 3 );
    }
    fprintf ( stderr, "Parse %s\n", argv[1] );
    //ret = TestParseItems2Safe ( argv[1] );
    ret = TestParseSessionFile ( argv[1] );

    return ret;
}




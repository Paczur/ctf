divert(-1)

define(`quote', `ifelse(`$#', `0', `', ``$*'')')
define(`dquote', ``$@'')
define(`dquote_elt', `ifelse(`$#', `0', `', `$#', `1', ```$1''',
                             ```$1'',$0(shift($@))')')
define(`foreach', `pushdef(`$1')_$0(`$1',
  (dquote(dquote_elt$2)), `$3')popdef(`$1')')
define(`_arg1', `$1')dnl
define(`_foreach', `ifelse(`$2', `(`')', `',
  `define(`$1', _arg1$2)$3`'$0(`$1', (dquote(shift$2)), `$3')')')
define(`PRIMITIVE_TYPES', ``char', `int', `uint', `ptr'')
define(`CMPS', ``eq', `neq', `lt', `gt', `lte', `gte'')
define(`COMB', `foreach(`x', `$2', `indir(`$1', x)
')')
define(`COMB2', `foreach(`x', `$2', `foreach(`y', `$3', `indir(`$1', x, y)
')')')
define(`COMB3', `foreach(`x', `$2', `foreach(`y', `$3',
`foreach(`z', `$4', `indir(`$1', x, y, z)
')')')')
define(`COMB4', `foreach(`x', `$2', `foreach(`y', `$3',
`foreach(`z', `$4', `foreach(`a', `$5', `indir(`$1', x, y, z, a)
')')')')')
define(`LQ',`changequote(<,>)`dnl'
changequote`'')
define(`RQ',`changequote(<,>)dnl`
'changequote`'')
define(`CMP_SYMBOL', `ifelse(`$1',eq,==,`$1',neq,!=,`$1',gt,>,`$1',lt,<,`$1',gte,>=,`$1',lte,<=)')
define(`FORMAT', `ifelse(`$1',char,`"'RQ()`%c'RQ()`"',`$1',int,"%jd",`$1',uint,"%ju",`$1',ptr,"%p",`$1',str,"\"%s\"")')
define(`TYPE', `ifelse(`$1',char,char,`$1',int,intmax_t,`$1',uint,uintmax_t,`$1',ptr,const void *,`$1',str,const char *)')
define(`SHORT', `ifelse(`$1',char,c,`$1',int,i,`$1',uint,u,`$1',ptr,p,`$1',str,p)')

divert(0)dnl

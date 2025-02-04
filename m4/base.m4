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

define(`PRIMITIVE_TYPES', ``char', `int', `uint', `ptr', `float'')
define(`SCOMB', `foreach(`x', `$3', `indir(`$1', `$2', x)
')')
define(`COMB', `foreach(`x', `$2', `indir(`$1', x)
')')
define(`COMB2', `foreach(`x', `$2', `foreach(`y', `$3', `indir(`$1', x, y)
')')')
define(`UP', `translit(`$*', `a-z', `A-Z')')
define(`COMB3', `foreach(`x', `$2', `foreach(`y', `$3',
`foreach(`z', `$4', `indir(`$1', x, y, z)
')')')')
define(`COMB4', `foreach(`x', `$2', `foreach(`y', `$3',
`foreach(`z', `$4', `foreach(`a', `$5', `indir(`$1', x, y, z, a)
')')')')')
define(`COMB5', `foreach(`x', `$2', `foreach(`y', `$3',
                                             `foreach(`z', `$4', `foreach(`a', `$5', 
                                                                          `foreach(`b', `$6', `indir(`$1', x, y, z, a, b)
                                                                                  ')')')')')')
define(`COMB6', `foreach(`x', `$2', `foreach(`y', `$3',
                                             `foreach(`z', `$4', `foreach(`a', `$5', 
                                                                          `foreach(`b', `$6', 
                                                                                   `foreach(`c', `$7', `indir(`$1', x, y, z, a, b, c)
                                                                                  ')')')')')')')
define(`TYPE', `ifelse(`$1',char,char,`$1',int,intmax_t,`$1',uint,uintmax_t,`$1',ptr,const void *,`$1',str,const char *,`$1',float,long double)')
define(`SHORT', `ifelse(`$1',char,c,`$1',int,i,`$1',uint,u,`$1',ptr,p,`$1',str,p,`$1',float,f)')
define(`RUN3', `format(`%s(%s, %s, %s, %s, %s, %s)', `$1', `$2', `UP(`$2')', `$3', `UP(`$3')', `$4', `UP(`$4')')')
define(`RUN2', `format(`%s(%s, %s, %s, %s)', `$1', `$2', `UP(`$2')', `$3', `UP(`$3')')')
define(`RUN1', `format(`%s(%s, %s)', `$1', `$2', `UP(`$2')')')

define(`EAS', `expect, assert')

divert(0)dnl

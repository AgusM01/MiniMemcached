-module(test).
-import(memcache, []).
-export([prueba/0]).

prueba() ->
    S = memcache:start(localhost),
    memcache:put(S, "Agustin", "Merino"),
    memcache:put(S, "Luciano", "Scola"),
    S.

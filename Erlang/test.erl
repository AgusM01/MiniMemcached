-module(test).
-import(memcache, []).
-import(memcache_txt, []).
-export([prueba/0, destruccion_total/1, readlines/1, destruccion_textual/1, test_uno/1]).


prueba() ->
    LoQueMando = readlines("bible.txt"),
    S = memcache:start(localhost),
    memcache:put(S, pruba, LoQueMando),
    memcache:get(S, pruba),
    memcache:del(S, pruba),
    memcache:close(S).


test_uno(N) ->
    S = memcache:start(localhost),
    genera_puts(S, N).

genera_puts(S, 0) ->
    memcache:put(S, 0, "estoycasadojefe");

genera_puts(S, N) ->
    case memcache:put(S, N - 1, "estoycansadojefe") of
        ok -> genera_puts(S, N - 1);
        Error -> {Error, memcache:close(S)}
    end.


destruccion_textual(0) ->
    S = memcache_txt:start(localhost),
    spawn(fun() -> proc_textualizado(S, 1) end);

destruccion_textual(N) ->
    S = memcache_txt:start(localhost),
    spawn(fun() -> proc_textualizado(S, N) end),
    destruccion_textual(N-1).


proc_textualizado(S, N) ->
    self() ! mandale,
    receive
        mandale -> 
            textos(S, N, N),
            proc_textualizado(S, N - 1);
        stop -> ok
    end.

textos(S, M, N) ->
    Clave = genera_texto( 97 + (N rem 26), M),
    Valor = genera_texto( 97 + (N rem 26), M),
    Put = "PUT " ++ Clave ++ " " ++ Valor ++ "\n",
    Get = "GET " ++ Clave ++ "\n",
    Del =  "DEL " ++ Clave ++ "\n",
    memcache_txt:send_msg(S, Put),
    memcache_txt:send_msg(S, Get),
    memcache_txt:send_msg(S, Del).


genera_texto(_, 0) ->
    [];

genera_texto(Txt, N) ->
    [Txt | genera_texto(Txt, N - 1)].


destruccion_total(0) ->
    S = memcache:start(localhost),
    spawn(fun() -> proc_incha(S, 1000) end);

destruccion_total(N) ->
    S = memcache:start(localhost),
    spawn(fun() -> proc_incha(S, 1000) end),
    destruccion_total(N - 1).

proc_incha(S, N) ->
    self() ! mandale,
    receive
        mandale -> 
            pedidos(S, self(), N, N),
            proc_incha(S, N);
        stop -> ok
    end.

pedidos(S, P, 0, N) ->
    memcache:put(S, {P, 0, ok}, <<0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0>>),
    memcache:get(S, {P, N, ok}),
    memcache:del(S, {P, N, ok}),
    memcache:put(S, {0, ok}, 0),
    memcache:get(S, {N, ok}),
    memcache:del(S, {N, ok});

pedidos(S, P, M, N) ->
    memcache:put(S, {P, M, ok}, <<0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0>>),
    memcache:get(S, {P, M - 1, ok}),
    memcache:del(S, {P, M - 1, ok}),
    memcache:put(S, {M, ok}, 0),
    memcache:get(S, {M - 1, ok}),
    memcache:del(S, {M - 1, ok}),
    pedidos(S, P, M - 1, N).

readlines(FileName) ->
    {ok, Device} = file:open(FileName, [read]),
    try get_all_lines(Device)
        after file:close(Device)
    end.

get_all_lines(Device) ->
    case io:get_line(Device, "") of
        eof -> [];
        Line -> Line ++ get_all_lines(Device)
    end.



-module(test).
-import(memcache, []).
-export([prueba/0]).

prueba() ->
    {ok, S} = gen_tcp:connect(localhost, 8889, [binary, {active, false}]),
    %BinToSend = memcache:encode_data(put, {"Agustin", "Merino"}),
    gen_tcp:send(S,<<11,0,0,0,2,22,22,0,0,0,3,33,33,33>>),
    gen_tcp:recv(S,1).

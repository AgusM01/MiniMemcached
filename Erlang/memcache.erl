-module(memcache).
-export([start/1,get/2,put/3,del/2,stats/1,encode_data/2]).

start(Host) ->
    {ok, S} = gen_tcp:connect(Host, 8889, [binary, {active, false}]),
    PidServer = spawn (fun() -> server(S) end),
    PidServer.

encode_to_big(N) ->
    Bin = binary:encode_unsigned(N),
    case byte_size(Bin) of
        1 -> list_to_binary([[0,0,0] | binary_to_list(Bin)]);
        2 -> list_to_binary([[0,0]   | binary_to_list(Bin)]);
        3 -> list_to_binary([[0]     | binary_to_list(Bin)]);
        4 -> Bin
    end.

decode_to_int(Bin) ->
    binary:decode_unsigned(Bin).

server(S) -> 
    receive
        {From, stats, nan} ->
            case gen_tcp:send(S, <<21>>) of
                ok -> From ! {self(), recv_cmd(S, stats)};
                Error -> From ! {self(), Error}
            end;
        {From, Cmd, Args} ->
            BinToSend = encode_data(Cmd, Args),
            case gen_tcp:send(S,BinToSend) of
                ok -> From ! {self(), recv_cmd(S, Cmd)};
                Error -> From ! {self(), Error}
            end
    end,
    server(S).


encode_data(put, Args) ->
    {Key, Value} = Args,
    BinKey = term_to_binary(Key),
    LenKey = encode_to_big(byte_size(BinKey)),
    BinValue = term_to_binary(Value),
    LenValue = encode_to_big(byte_size(BinValue)),

    list_to_binary([11,LenKey,BinKey,LenValue,BinValue]);

encode_data(Cmd, Key) ->
    case Cmd of
        del -> N = 12;
        get -> N = 13
    end,
    BinKey = term_to_binary(Key),
    LenKey = encode_to_big(byte_size(BinKey)),
    list_to_binary([N,LenKey,BinKey]).


recv_cmd(S, stats) ->
    io:format("HOla"),
    case gen_tcp:recv(S, 1) of
        {ok, <<101>>} -> recv_len(S);
        Error -> Error
    end; 

recv_cmd(S, put) ->
    io:format("rcv cmd put~n"),
    case gen_tcp:recv(S,1) of
        {ok, <<101>>} -> ok;
        Error -> Error
    end;

recv_cmd(S, Cmd) ->
    io:format("rcv cmd ~n"),
    case gen_tcp:recv(S, 1) of
        {ok, <<101>>} -> 
            case Cmd of
            get -> recv_len(S);
            del -> ok
            end;
        {ok, <<112>>} -> {error, enotfound};
        Error -> Error
    end.

recv_len(S) ->
    io:format("Recv_len~n"),
    case gen_tcp:recv(S, 4) of
        {ok, BinLen} -> gen_tcp:recv(S, decode_to_int(BinLen));
        Error -> Error
    end.

put(PidServer, Key, Value) ->
    PidServer ! {self(), put, {Key, Value}},
    receive
        {PidServer, ok} -> ok;
        Error -> Error
    end.

get(PidServer, Key) ->
    PidServer ! {self(), get, Key},
    receive
        {PidServer, {ok, Value}} -> binary_to_term(Value);
        Error -> Error
    end.

del(PidServer, Key) ->
    PidServer ! {self(), get, Key},
    receive
        {PidServer, ok} -> ok;
        Error -> Error
    end.

stats(PidServer) ->
    PidServer ! {self(), stats},
    receive
        {PidServer, {ok, Stats}} -> binary_to_term(Stats);
        Error -> Error
    end.
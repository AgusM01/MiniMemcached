-module(memcache).
-export([connect_server/1]).

connect_server(Host) ->
    {ok, S} = gen_tcp:connect(Host, 8889, [binary, {active, false}]),
    PidServer = spawn (fun() -> server(S) end),
    PidServer.



get_cmd(S, Msg) ->
    case Msg of
        <<Cmd:1,Rest/bitstring>> ->
            {Cmd, Rest};
        <<Cmd:1>> ->
            receive
                {tcp, S, }


int_bin_rep(N) ->
    List = int_bin_rep(N,[]),
    list_to_binary(list).

%int_bin_rep(N, List) ->





encode_big(N) ->
    Bin = binary:decode_unsigned(N),
    case byte_size(Bin) of
        1 -> list_to_binary([[0,0,0] | binary_to_list(Bin)]);
        2 -> list_to_binary([[0,0]   | binary_to_list(Bin)]);
        3 -> list_to_binary([[0]     | binary_to_list(Bin)]);
        4 -> Bin
    end.


server(S) -> 
    receive
        {From, stats, nan} ->
            gen_tcp:send(S, <<21>>);
        {From, Cmd, Args} ->
            case get_data(S, Key) of
                {ok, Msg} -> From ! {self(), Msg};
                einval -> From ! einval
            end
        end.
encode_data(put, {Key, Value}) ->
    Cmd = 11,
    BinKey = term_to_binary(Key),
    LenKey = encode_big(byte_size(BinKey)),
    BinValue = term_to_binary(Value),
    LenValue = encode_big(byte_size(BinValue)),
    list_to_binary([Cmd,LenKey,BinKey,LenValue,BinValue]);

encode_data(stats, Key) ->
    Cmd = 12,
    BinKey = term_to_binary(Key),
    LenKey = encode_big(byte_size(BinKey)),
    list_to_binary([Cmd,LenKey,BinKey])




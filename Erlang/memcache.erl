-module(memcache).
-export([start/1,get/2,put/3,del/2,stats/1,close/1]).

%start :: SockAddr -> pid()
% Genera la conexión del servidor en la dirección SockAddr
% en modo binario. Devuelve el identificador del proceso que
% maneja los pedidos al server.
start(Host) ->
    {ok, S} = gen_tcp:connect(Host, 8889, [binary, {active, false}]),
    PidServer = spawn (fun() -> server(S) end),
    PidServer.

%server :: Socket 
% Loop que maneja los pedidos al servidor de los clientes.
% La idea es que un cliente pueda tener varios procesos accediendo
% a la misma conexión y que el proceso server los vaya responediendo.
server(S) -> 
    receive
        {From, Cmd, Args} ->
            BinToSend = encode_data(Cmd, Args),
            case gen_tcp:send(S,BinToSend) of
                ok ->
                    From ! {self(), recv_cmd(S, Cmd)};
                Error -> 
                    From ! {self(), Error}
            end,
            server(S);
        {From, close} ->
            From ! {self(), gen_tcp:close(S)}
    end.

% put :: {pid(), Term, Term} -> ok | Error
put(PidServer, Key, Value) ->
    PidServer ! {self(), put, {Key, Value}},
    receive
        {PidServer, ok} -> ok;
        Error -> Error
    end.

% get :: {pid(), Term} -> Term | Error
get(PidServer, Key) ->
    PidServer ! {self(), get, Key},
    receive
        {PidServer, {ok, Value}} -> binary_to_term(Value);
        Error -> Error
    end.

% del :: {pid(), Term} -> ok | Error
del(PidServer, Key) ->
    PidServer ! {self(), del, Key},
    receive
        {PidServer, ok} -> ok;
        Error -> Error
    end.

% stats :: pid() -> String
stats(PidServer) ->
    PidServer ! {self(), stats, nan},
    receive
        {PidServer, {ok, Stats}} -> binary_to_list(Stats);
        Error -> Error
    end.

% close :: pid() -> ok
close(PidServer) ->
    PidServer ! {self(), close},
    receive 
        {PidServer, ok} -> ok
    end.

% encode_data :: {Atom, Atom} | {Atom, Term} | {Atom, {Term, Term}} -> binary()
% Codifica, según el commando pasado como primer argumento, la información en 
% binario que posteriormente será enviada al servidor de la memcache.
% Se hace pattern matching sobre los comandos.

encode_data(stats, nan) ->
    <<21>>;

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


% recv_cmd :: {Socket, Atom} -> {ok, binary()} | Error
% Recibe el header del comando que especifica si el pedido
% fué enviado de manera correcta. Luego dependiendo del comando
% se lee el resto del pedido.
% Se hace pattern matching sobre los comandos.
recv_cmd(S, stats) ->
    case gen_tcp:recv(S, 1) of
        {ok, <<101>>} -> recv_len(S);
        Error -> Error
    end; 

recv_cmd(S, put) ->
    case gen_tcp:recv(S,1) of
        {ok, <<101>>} -> ok;
        Error -> Error
    end;

recv_cmd(S, Cmd) ->
    case gen_tcp:recv(S, 1) of
        {ok, <<101>>} -> 
            case Cmd of
            get -> recv_len(S);
            del -> ok
            end;
        {ok, <<112>>} -> {error, enotfound};
        Error -> Error
    end.

%recv_len :: Socket -> {ok, binary()}
% Recibe los 4 bytes correspondientes a la longitud del 
% binario del resto del pedirlo posteriormente.
recv_len(S) ->
    case gen_tcp:recv(S, 4) of
        {ok, BinLen} -> gen_tcp:recv(S, decode_to_int(BinLen));
        Error -> Error
    end.

% encode_to_big :: integer() -> binary()
% Codifica el entero pasado como argumento en formato binario
% Big-Endian unsigned.
encode_to_big(N) ->
    Bin = binary:encode_unsigned(N),
    case byte_size(Bin) of
        1 -> list_to_binary([[0,0,0] | binary_to_list(Bin)]);
        2 -> list_to_binary([[0,0]   | binary_to_list(Bin)]);
        3 -> list_to_binary([[0]     | binary_to_list(Bin)]);
        4 -> Bin
    end.

% decode_to_int :: binary() -> integer()
% Convierte un binario en formato Big-Endin unsigned a integer()
decode_to_int(Bin) ->
    binary:decode_unsigned(Bin).
let usage_msg = "clone [options] [program [arguments]]"

let ipc = ref false
let mount = ref false
let network = ref false
let pid = ref false
let uts = ref false
let user = ref false
let cgroup = ref false
let time = ref false

let verbose = ref false

let program_args = ref []
let anon_fun arg =
  program_args := arg::!program_args

let speclist = [
  ("-i", Arg.Set ipc,     "Clone into a new IPC namespace");
  ("-m", Arg.Set mount,   "Clone into a new mount namespace");
  ("-n", Arg.Set network, "Clone into a new network namespace");
  ("-p", Arg.Set pid,     "Clone into a new PID namespace");
  ("-u", Arg.Set uts,     "Clone into a new UTS (Unix Time Sharing) namespace");
  ("-U", Arg.Set user,    "Clone into a new user namespace");
  ("-C", Arg.Set cgroup,  "Clone into a new IPC namespace");
  ("-T", Arg.Set time,    "Clone into a new time namespace");

  ("-v", Arg.Set verbose, "Verbose logging")
]

let veprintf = if !verbose then Printf.eprintf else Printf.eprintf

type pid_t = int

type clone_args = {
  (** Flags for the clone call *)
  flags: int;

  (** Pointer to where to store the pidfd *)
  pidfd: pid_t ref option;

  (** Where to place the child thread ID in the child's memory *)
  child_tid: pid_t ref option;

  (** Where to place the child thread ID in the parent's memory *)
  parent_tid: pid_t ref option;

  (** Signal to deliver to parent on child's termination *)
  exit_signal: int;

  (** Stack for the child if the parent and child share memory *)
  stack: bytes ref;

  (* stack_size: included in stack *)

  (** Location of new thread local storage *)
  tls: int ref;

  (** Optional list of specific pids for one or more of the namespaces *)
  set_tid: pid_t list option;

  (* set_tid_size: included in set_tid *)
}

external clone3: unit -> unit = "test"

let () =
  Arg.parse speclist anon_fun usage_msg;

  if !ipc then begin
    veprintf "cloning into a new IPC namespace\n";
  end;

  if !mount then veprintf "cloning into a new mount namespace\n";
  if !network then veprintf "cloning into a new network namespace\n";
  if !pid then veprintf "cloning into a new PID namespace\n";
  if !uts then veprintf "cloning into a new UTS namespace\n";
  if !user then veprintf "cloning into a new user namespace\n";
  if !cgroup then veprintf "cloning into a new cgroup namespace\n";
  if !time then veprintf "cloning into a new time namespace\n";

  clone3 ();

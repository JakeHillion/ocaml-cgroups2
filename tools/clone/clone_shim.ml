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

let anon_fun arg = program_args := arg :: !program_args

let speclist =
  [ ("-i", Arg.Set ipc, "Clone into a new IPC namespace")
  ; ("-m", Arg.Set mount, "Clone into a new mount namespace")
  ; ("-n", Arg.Set network, "Clone into a new network namespace")
  ; ("-p", Arg.Set pid, "Clone into a new PID namespace")
  ; ("-u", Arg.Set uts, "Clone into a new UTS (Unix Time Sharing) namespace")
  ; ("-U", Arg.Set user, "Clone into a new user namespace")
  ; ("-C", Arg.Set cgroup, "Clone into a new IPC namespace")
  ; ("-T", Arg.Set time, "Clone into a new time namespace")
  ; ("-v", Arg.Set verbose, "Verbose logging") ]

let veprintf = if !verbose then Printf.eprintf else Printf.eprintf

type pid_t = int

type clone_flag =
  | CLONE_CHILD_CLEARTID
  | CLONE_CHILD_SETTID
  | CLONE_CLEAR_SIGHAND
  | CLONE_FILES
  | CLONE_FS
  | CLONE_IO
  | CLONE_NEWCGROUP
  | CLONE_NEWIPC
  | CLONE_NEWNET
  | CLONE_NEWNS
  | CLONE_NEWPID
  | CLONE_NEWUSER
  | CLONE_NEWUTS
  | CLONE_PARENT
  | CLONE_PARENT_SETTID
  | CLONE_PIDFD
  | CLONE_PTRACE
  | CLONE_SETTLS
  | CLONE_SIGHAND
  | CLONE_SYSVSEM
  | CLONE_THREAD
  | CLONE_UNTRACED
  | CLONE_VFORK
  | CLONE_VM
  | CLONE_NEWTIME

type clone_args =
  { flags: clone_flag list  (** Flags for the clone call *)
  ; pidfd: pid_t ref option  (** Pointer to where to store the pidfd *)
  ; child_tid: pid_t ref option
        (** Where to place the child thread ID in the child's memory *)
  ; parent_tid: pid_t ref option
        (** Where to place the child thread ID in the parent's memory *)
  ; exit_signal: int
        (** Signal to deliver to parent on child's termination *)
  ; stack: bytes ref option
        (** Stack for the child if the parent and child share memory *)
  ; tls: int ref option  (** Location of new thread local storage *)
  ; set_tid: pid_t list option
        (** Optional list of specific pids for one or more of the namespaces *)
  }

external clone3 : clone_args -> (unit -> unit) -> unit = "caml_clone3"

let hello_world_caml unit = Printf.eprintf "hello world from ocaml\n"

let () =
  let flags = ref [] in
  Arg.parse speclist anon_fun usage_msg ;
  if !ipc then (
    veprintf "cloning into a new IPC namespace\n" ;
    flags := CLONE_NEWIPC :: !flags ) ;
  if !mount then (
    veprintf "cloning into a new mount namespace\n" ;
    flags := CLONE_NEWNS :: !flags ) ;
  if !network then (
    veprintf "cloning into a new network namespace\n" ;
    flags := CLONE_NEWNET :: !flags ) ;
  if !pid then (
    veprintf "cloning into a new PID namespace\n" ;
    flags := CLONE_NEWPID :: !flags ) ;
  if !uts then (
    veprintf "cloning into a new UTS namespace\n" ;
    flags := CLONE_NEWUTS :: !flags ) ;
  if !user then (
    veprintf "cloning into a new user namespace\n" ;
    flags := CLONE_NEWUSER :: !flags ) ;
  if !cgroup then (
    veprintf "cloning into a new cgroup namespace\n" ;
    flags := CLONE_NEWCGROUP :: !flags ) ;
  if !time then (
    veprintf "cloning into a new time namespace\n" ;
    flags := CLONE_NEWTIME :: !flags ) ;
  clone3
    { flags= !flags
    ; pidfd= None
    ; child_tid= None
    ; parent_tid= None
    ; exit_signal= 0
    ; stack= None
    ; tls= None
    ; set_tid= None }
    hello_world_caml ;
  veprintf "exiting now\n"

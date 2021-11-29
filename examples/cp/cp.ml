open Printf

let usage_msg = "append [-verbose] <file1> [<file2>] ... -o <output>"

let files = ref []
let anon_fun filename = files := filename::!files

let () =
  Arg.parse [] anon_fun usage_msg;

  let src_file = List.hd !files in
  files := List.tl !files;

  let dst_file = List.hd !files in
  files := List.tl !files;

  let ic = open_in src_file in
  let oc = open_out dst_file in

  
  let buf = Bytes.create 4096 in

  let read = ref 4096 in

  while !read > 0 do
    (* input ic buf pos len *)
    read := input ic buf 0 4096;
    (* output oc buf pos len *)
    output oc buf 0 !read
  done

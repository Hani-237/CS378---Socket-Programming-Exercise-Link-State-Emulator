import java.io.*;
import java.util.*;
import java.net.*;

class vnode{
    char ch;
    Socket s;
    DataInputStream Input;
    DataOutputStream Output;
    int port;
    byte[] ip;
}

class on{
    public static void main(String[] args) throws IOException{
        ArrayList<int[]> nodes=new ArrayList<>();
        if(args.length!=1){
            throw new IllegalArgumentException("Invalid input: Number of arguments should be 1");
        }
        try(Scanner sc=new Scanner(new File(args[0]))){
            int k=0;
            while(sc.hasNextLine()){
                String line=sc.nextLine().trim();
                if(line.isEmpty() || line.startsWith("#")){
                    continue;                     
                }
                String[] parts=line.split("\\s+");
                int[] arr=new int[parts.length];
                for(int i=0;i<parts.length;i++) {
                    arr[i]=Integer.parseInt(parts[i]);
                }
                if(k>0){
                    if(nodes.get(k-1).length-1!=arr.length){
                        throw new IllegalArgumentException("Invalid input: Input Format Mismatch, each line should have exactly one less input than the previous line");
                    }
                }
                nodes.add(arr);
                k++;
            }
            if(!nodes.isEmpty() && nodes.get(nodes.size()-1).length!=1){
                throw new IllegalArgumentException("Invalid input: Input Format Mismatch, Last line must have single input");
            }
        }
        ServerSocket server=new ServerSocket(5000);
        int no_of_nodes=nodes.size()+1,n=no_of_nodes;
        char ch='A';
        ArrayList<vnode> vnodes=new ArrayList<>();
        while(n-->0){
            vnode node=new vnode();
            Socket s=server.accept();
            DataInputStream in=new DataInputStream(s.getInputStream());
            byte[] ip=new byte[4];
            ip=in.readNBytes(4);
            if(ip.length<4){
                //not enough bytes recieved
            }
            int port=in.readUnsignedShort();
            node.ch=(char)(ch+no_of_nodes-n-1);
            node.s=s;
            node.Input=in;
            node.ip=ip;
            node.port=port;
            vnodes.add(node);
        }
        //what about extra nodes
        server.close();
    }
}
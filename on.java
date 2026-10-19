import java.io.*;
import java.util.*;
import java.net.*;

class vnode{
    int[] adjacency;
    char ch;
    Socket s;
    DataInputStream Input;
    DataOutputStream Output;
    int port;
    byte[] ip;
}

class on{
    public static void main(String[] args) throws IOException{
        ArrayList<int[]> intial_connections=new ArrayList<>();
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
                    if(intial_connections.get(k-1).length-1!=arr.length){
                        throw new IllegalArgumentException("Invalid input: Input Format Mismatch, each line should have exactly one less input than the previous line");
                    }
                }
                intial_connections.add(arr);
                k++;
            }
            if(!intial_connections.isEmpty() && intial_connections.get(intial_connections.size()-1).length!=1){
                throw new IllegalArgumentException("Invalid input: Input Format Mismatch, Last line must have single input");
            }
        }
        int no_of_nodes=intial_connections.size()+1,n=no_of_nodes;
        int [][] connections=new int[no_of_nodes][no_of_nodes];
        for(int i=0;i<n;i++){
            for(int j=0;j<n;j++){
                if(j>i){
                    connections[i][j]=intial_connections.get(i)[j-1];
                }
                else if(i==j){
                    connections[i][j]=0;
                }
                else{
                    connections[i][j]=connections[j][i];
                }
            }
        }
        ServerSocket server=new ServerSocket(5000);
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
            node.adjacency=connections[node.ch-'A'];
            vnodes.add(node);
        }
        //what about extra nodes

        for(vnode node:vnodes){
            int neighbours=0;
            int[] ad=node.adjacency;
            for(int i=0;i<no_of_nodes;i++){
                if(ad[i]!=-1){
                    neighbours++;
                }
            }

        }

        server.close();
    }
}
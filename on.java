import java.io.*;
import java.util.*;
import java.net.*;

class vnode{
    int[] adjacency;
    char Name;
    Socket s;
    int port;
    byte[] ip;
    int neighbours;
    public void neighbour_update(){
        neighbours=0;
        if(adjacency.length==0){
            neighbours=1;
        }
        else{
            for(int i=0;i<adjacency.length;i++){
                if(adjacency[i]!=-1){
                    neighbours++;
                }
            }
        }    
    }
    public void input() throws IOException{
        DataInputStream in=new DataInputStream(s.getInputStream());
        ip=in.readNBytes(4);
        if(ip.length<4){
            throw new IllegalArgumentException("ipaddr length is not 4 Bytes");
        }
        port=in.readUnsignedShort();
    }
    public void output(vnode[] vnodes) throws IOException{
        DataOutputStream out=new DataOutputStream(s.getOutputStream());
        out.writeInt(neighbours);
        if(adjacency.length==0){
            out.writeByte(Name);
            out.write(ip);
            out.writeShort(port);
            out.writeInt(0);
        }
        else{
            for(int i=0;i<adjacency.length;i++){
                if(adjacency[i]!=-1){
                    out.writeByte('A'+i);
                    out.write(vnodes[i].ip);
                    out.writeShort(vnodes[i].port);
                    out.writeInt(adjacency[i]);
                }
            }
        }
        
        out.flush();
    }
}

class on{
    static int[][] parser(String path) throws IOException{
        ArrayList<int[]> intial_connections=new ArrayList<>();
        try(Scanner sc=new Scanner(new File(path))){
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
                    connections[i][j]=intial_connections.get(i)[j-i-1];
                }
                else if(i==j){
                    connections[i][j]=0;
                }
                else{
                    connections[i][j]=connections[j][i];
                }
            }
        }
        return connections;
    }
    public static void main(String[] args) throws IOException,InterruptedException{
        if(args.length!=1){
            throw new IllegalArgumentException("Invalid input: Number of arguments should be 1");
        }
        File cfg=new File(args[0]);
        long old_file=cfg.lastModified();
        int[][] connections=parser(args[0]);
        int no_of_nodes=connections.length;
        if(no_of_nodes>26){
            throw new IllegalArgumentException("Number of nodes should be less than 26");
        }
        vnode[] vnodes=new vnode[26];
        for(int i=0;i<26;i++){
            vnodes[i]=new vnode();
            vnodes[i].Name=(char)('A'+i);
        }
        ServerSocket server=new ServerSocket(5000);
        server.setSoTimeout(20000);
        int p=0;
        while(true){
            try{
                if(p<no_of_nodes){
                    Socket s=server.accept();
                    vnodes[p].s=s;
                    vnodes[p].input();
                    vnodes[p].adjacency=connections[p];
                    p++;
                }
                else{
                    Socket s=server.accept();
                    vnodes[p].s=s;
                    vnodes[p].input();
                    vnodes[p].adjacency=new int[0];
                    p++;
                }
                if(p==26){
                    break;
                }
            }
            catch(SocketTimeoutException e){
                break;
            }
        }
        if(p<no_of_nodes){
            throw new IllegalArgumentException("Not enough VN nodes");
        }
        for(int i=0;i<p;i++){
            vnodes[i].neighbour_update();
            vnodes[i].output(vnodes);
        }
        while(true){
            Thread.sleep(1000);
            long new_file=cfg.lastModified();
            if(new_file!=old_file){
                int[][] new_connections=parser(args[0]);
                if(new_connections.length!=no_of_nodes || new_connections[0].length!=no_of_nodes){
                    throw new IllegalArgumentException("new and old number of nodes doesn't match");
                }
                for(int i=0;i<p;i++){
                    if(i<no_of_nodes){
                        vnodes[i].adjacency=new_connections[i];
                    }
                    else{
                        vnodes[i].adjacency=new int[0];
                    }
                    vnodes[i].neighbour_update();
                    vnodes[i].output(vnodes);
                }
                old_file=new_file;
            }
        }
    }
}
import java.io.File;
import java.io.FileNotFoundException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Scanner;

class on{
    public static void main(String[] args) throws FileNotFoundException {
        Scanner sc=new Scanner(new File("oracle-node-config.txt"));
        ArrayList<int[]> nodes=new ArrayList<>();
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
            nodes.add(arr);
        }
        sc.close();
        int no_of_nodes=nodes.size()+1;
        // while(no_of_nodes-->0){

        // }
    }
}
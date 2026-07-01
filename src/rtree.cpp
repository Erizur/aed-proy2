#include "rtree.hpp" 
#include <limits> //Para usar infinito y comparar minimos

MBR pointMBR(double x, double y){
    return {x,y,x,y}; //Punto que se representa como rectangulo
}

RTree::RTree(int maxEntries){
    maxSize=maxEntries;
    minSize=(maxSize+1)/2;//Minimo de entradas por nodo
    root=new RTNode(); //Crea la raiz inicial
    root-> isLeaf=true;//Al inicio la raiz es hoja
    root->parent=nullptr;//La raiz no tiene padre
}

void RTree::updateMBR(RTNode* node){
    if(node==nullptr|| node->entries.empty()) return; //No actualiza nodos nulos o vacios

    MBR current = node->entries[0].mbr; //Empieza con el MBR de la primera entrada

    for (int i=1;i<node->entries.size();i++){//Recorre las demas entradas del nodo
        current=current.expand(node->entries[i].mbr);//Expande el MBR acumulado
    }
    node->mbr=current; //Guarda el MBR total del nodo
}

RTNode* RTree::chooseLeaf(const MBR& mbr){
    RTNode* current=root;//La busqueda empieza desde la raiz

    while(current != nullptr && !current->isLeaf){//While que baja hasta encontrar una hoja
        int bestIndex=0;
        double bestEnlarge = std::numeric_limits<double>::infinity();//Mejor crecimiento encontrado
        double bestArea=std::numeric_limits<double>::infinity();//Mejor area de desempate

        for(int i=0;i<current->entries.size();++i){//Mira cada entrada del nodo actual
            double enlarge=current->entries.[i].mbr.enlarge(mbr);//Calcula cuanto crecera ese hijo
            double area=current->entries[i].mbr.area();//Calcula el area actual del hijo
            if (enlarge<bestEnlarge || (enlarge==bestEnlarge && area<bestArea)){//Aplica el criterio de menor crecimiento
                bestIndex=i;//Actualiza el menor hijo
                bestEnlarge=enlarge;//Guarda el menor crecimiento
                bestArea=area;//Guarda el area usada para desempate
            }
        }
        current=current->entries[bestIndex].child;//Baja al hijo elegido.
    }
    return current;//Retornna la hoja donde insertara
}

void RTree::adjustTree(RTNode* node, RTNode* newNode){
    while (node!=nullptr){//Sube desde el nodo modificado hasta la raiz
        updateMBR(node);//Recalcula el MBR del nodo actual
        if (newNode !=nullptr){//Si hubo split, se debera insertar newNode en el padre
            //Falta implementar split
        }
        node=node->parent;//Sube al padre
    }
}
void RTree::insert(int id, const std::string& name,double x,double y, const std::string& category){
    MBR mbr= pointMBR(x,y); //Crea MBR degenerado del punto
    SpatialObject object;//Crea objeto espacial y vamos a guardar todos sus datos
    object.id=id;
    object.name=nameobject.category=category;
    object.x=x;
    object.y=y;

    Entry entry;// Crea una entrada para la hoja
    entry.mbr = mbr;//La entrada solo cubrira un punto
    entry=child = nullptr;
    entry.object=object;//Guarda el objeto real
    entry.hasObject =true;

    RTNode* leaf =chooseLeaf(mbr);//Busca la mejor hoja para insertar
    leaf->entries.push_back(entry);//Inserta la entrada en la hoja
    adjustTree(leaf,nullptr);//Actualiza MBRs hacia arriba

    if(leaf->entries.size()>maxize){//Detecta overflow del nodo hoja
        //Falta implementar splitNode
    }
Vector<Entry> RTree::rangeQuery(double minX,double minY,double maxX, double maxY){
    Vector<Entry> result;//Guardara las entradas encontradas
    MBR query={minX,minY,maxX,maxY};//Crea ek rectangulo de consulta
    if (root=nullptr || root->entries.empty()) return result;//Si el arbol esta vacio
    Vector<RTNode*>stack;//Usamos un vector como pila de nodos por visitar
    stack.push_back(root);//Empezamos desde la raiz
    while(!stack.empty()){
        RTNode* node=stack[stack.size()-1];//Tomamos el ultimo nodo agregado
        stack.pop_back();

        for (int i=0; i<node->entries.size();++i){
            Entry& entry=node->entries[i]; //Hacemos referencia a la entrada actual

            if(!entry.mbr.intersects(query))continue;//Si no intersecta la consulta, se poda
            if(node->isLeaf){//Si estamos en una hoja, la entrada es un objeto
                if(query.containsPoint(entry.object.x,entry.object.y)){//Verifica que el punto este dentro
                    result.push_back(entry);//Agregamos el objeto al resultado
                }
                
            }else{
                    stack.push_back(entry.child);//Agrega el hijo para revisarlo luego
            }
        }
    }
    return result;
}

Vector<Entry> RTree::knn(double x,double y, int k){//Falta terminar
    Vector<Entry> result;
    return result;
}

}

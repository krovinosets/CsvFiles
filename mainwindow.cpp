#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "QMessageBox"
#include <QFileDialog>
#include <QPainter>
#include <QGraphicsScene>
#include <set>
#include <vector>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow){
    ui->setupUi(this);
    this->setWindowTitle("Csv files");
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_btn_choseFileName_clicked()
{
    QString filename = QFileDialog::getOpenFileName(this, "Choose file", "", "*.csv");
    key=0;
    ui->lbl_filename->setText(filename);
    ui->btn_Load_data->setEnabled(true);
    ui->btn_calc_metrics->setEnabled(false);
    ui->box_column->clear();
    ui->box_region->clear();
    ui->tb_widget->setColumnCount(0);
    ui->tb_widget->setRowCount(0);
    ui->tb_widget->clearContents();
    ui->lbl_min->setText("Min value: ");
    ui->lbl_max->setText("Max value: ");
    ui->lbl_median->setText("Median value: ");
    if (filename.isEmpty())
        QMessageBox::critical(this,"Error","You should chose file");
}

void MainWindow::on_btn_Load_data_clicked()
{
    QString filename=ui->lbl_filename->text();
    if (!filename.isEmpty()){
        FuncArgument fa = {.filename = QstringToCharArray(filename)};
        FuncReturningValue* frv = entryPoint(getData, &fa);
        if (frv->error==FILE_OPEN_ERROR)
            QMessageBox::information(this,"Error","There are problems with opening the file");
        else{
            showData(frv);
            FuncArgument fa2 = {
                .filename = fa.filename,
                .data = frv->data,
                .headers = frv->headers,
                .len = frv->len,
                .fields_num = frv->fields_num
            };
            key=1;
            entryPoint(cleanData, &fa2);
            free(frv);
        }
        ui->btn_Load_data->setEnabled(false);
        ui->btn_calc_metrics->setEnabled(true);
    }
}

void MainWindow::on_btn_calc_metrics_clicked()
{
    on_btn_Load_data_clicked();
    size_t index_of_column=(size_t)ui->box_column->currentIndex();
    FuncArgument fa = {
            .filename=QstringToCharArray(ui->lbl_filename->text()),
            .data = getDataFromTable(),
            .region=QstringToCharArray(ui->box_region->currentText()),
            .column=index_of_column+calculateColumns(index_of_column),
            .len = (size_t)ui->tb_widget->rowCount(),
            .fields_num = (size_t)ui->tb_widget->columnCount(),
            .region_number=(size_t)ui->box_region->currentIndex(),
            .region_index_at_header=(size_t)headers.indexOf("region")
    };
    FuncReturningValue* frv = entryPoint(calculateData, &fa);
    showData(frv);
    if (frv->error!=CALCULATE_ERROR){
        ui->lbl_min->setText("Min value: " + QString::number(frv->solution_min));
        ui->lbl_max->setText("Max value: " + QString::number(frv->solution_max));
        ui->lbl_median->setText("Median value: " + QString::number(frv->solution_median));
        draw(frv);
        entryPoint(cleanData, &fa);
    }
    else{
        QMessageBox::critical(this,"Error","It's impossible to calculate metrics for this column");
        ui->lbl_min->setText("Min value: ");
        ui->lbl_max->setText("Max value: ");
        ui->lbl_median->setText("Median value: ");
    }
    free(frv);
}

void MainWindow::on_box_region_currentTextChanged()
{
    if (key==1)
    {
        ui->tb_widget->setColumnCount(0);
        ui->tb_widget->setRowCount(0);
        ui->tb_widget->clearContents();
        ui->lbl_min->setText("Min value: ");
        ui->lbl_max->setText("Max value: ");
        ui->lbl_median->setText("Median value: ");
    }
}


void MainWindow::on_box_column_currentTextChanged()
{
    if (key==1)
    {
        ui->lbl_min->setText("Min value: ");
        ui->lbl_max->setText("Max value: ");
        ui->lbl_median->setText("Median value: ");
    }
}

size_t MainWindow::calculateFloor(vector<double> vec){
    size_t floor = 0;
    for(size_t i = 0; i < vec.size()-1; i++){
        if(vec[i] < 0 && vec[i+1] > 0){
            floor = vec.size() - i+1;
            break;
        } else if(vec[i] == 0){
            floor = (vec.size() - i);
            break;
        }else if(vec[i+1] == 0){
            floor = ((vec.size() - i) + 1);
            break;
        }
    }
    return floor;
}

void MainWindow::draw(FuncReturningValue*frv)
{
    const int spectrWidth = 741;
    const int spectrHeight = 251;
    QPixmap spectrPixmap(spectrWidth, spectrHeight);
    QPainter p(&spectrPixmap);
    QStringList years = getYears();
    QStringList nums = getNums(frv);
    std::set<QString> ySize;
    std::vector<double> vec;
    bool t = false;
    for(int i = 0; i < years.size(); i++){
        QString st = nums[i];
        ySize.insert(st);
        if(st.toDouble() < 0){
            t = true;
        }
    }
    for(auto i = ySize.begin(); i != ySize.end(); ++i)
        vec.push_back(i->toDouble());
    std::sort(vec.begin(), vec.end());
    size_t floor = 0;
    if(t){
        floor = calculateFloor(vec);
    }
    double mX = spectrWidth / years.size();
    double mY = spectrHeight / ySize.size();
    p.eraseRect(0,0,741,251); // очищаем рисунок
    double y_start = !t ? spectrHeight-20 : floor*mY;
    p.drawLine(50, y_start, spectrWidth, y_start); // ox
    p.drawLine(50, 20, 50, spectrHeight-20); // oy
    int id = 0;
    for(double i = 50; i <= spectrWidth; i += mX){ // рисуем черточки на координатой оси
        p.setPen(QPen(Qt::black,3));
        p.drawPoint(i, y_start);
        p.setPen(QPen(Qt::black,1));
        p.drawText(QPoint(i, y_start), years[id]);
        ++id;
    }
    id = 1;
    for(double i = 20; i <= spectrHeight-20; i += mY){ // рисуем черточки на координатой оси
        p.setPen(QPen(Qt::black,3));
        p.drawPoint(50, i);
        p.setPen(QPen(Qt::black,1));
        p.drawText(QPoint(4,i), QString::number(vec[vec.size() - id]));
        ++id;
    }
    int y;
    int ind = 0;
    int prevX = 0, prevY = 0;
    for (int x = 50 + mX; x <= spectrWidth; x += mX)
    {
        for(size_t i = 0; i < vec.size(); i++){
            if(nums[i] != QString("deleted") && vec[i] == nums[ind].toDouble()){
                if(t)
                    y = (mY*(vec.size() - i + 2));
                else
                    y = y_start - (mY*(i+1));
                nums[i] = "deleted";
                break;
            }
        }
        if(prevX != 0){
            p.setPen(QPen(Qt::green,1));
            p.drawLine(prevX, prevY, x, y);
        }
        p.setPen(QPen(Qt::red,4));
        p.drawPoint(x, y);
        prevX = x, prevY = y;
        ++ind;
    }
    ui->lbl_graphic->setPixmap(spectrPixmap);
}

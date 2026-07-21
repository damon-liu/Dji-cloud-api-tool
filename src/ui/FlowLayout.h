#ifndef FLOWLAYOUT_H
#define FLOWLAYOUT_H

#include <QLayout>
#include <QStyle>
#include <QWidget>
#include <QList>

class FlowLayout : public QLayout {
public:
    explicit FlowLayout(QWidget* parent = nullptr, int margin = 0, int hSpacing = 6, int vSpacing = 6)
        : QLayout(parent), mHSpace(hSpacing), mVSpace(vSpacing)
    {
        setContentsMargins(margin, margin, margin, margin);
    }

    ~FlowLayout() override {
        QLayoutItem* item;
        while ((item = takeAt(0)))
            delete item;
    }

    void addItem(QLayoutItem* item) override {
        mItems.append(item);
    }

    int count() const override { return mItems.size(); }

    QLayoutItem* itemAt(int index) const override {
        return mItems.value(index);
    }

    QLayoutItem* takeAt(int index) override {
        return index >= 0 && index < mItems.size() ? mItems.takeAt(index) : nullptr;
    }

    Qt::Orientations expandingDirections() const override { return {}; }

    bool hasHeightForWidth() const override { return true; }

    int heightForWidth(int width) const override {
        return doLayout(QRect(0, 0, width, 0), true);
    }

    QSize minimumSize() const override {
        QSize size;
        for (const QLayoutItem* item : mItems)
            size = size.expandedTo(item->minimumSize());
        const QMargins m = contentsMargins();
        size += QSize(m.left() + m.right(), m.top() + m.bottom());
        return size;
    }

    QSize sizeHint() const override {
        return minimumSize();
    }

    void setGeometry(const QRect& rect) override {
        QLayout::setGeometry(rect);
        doLayout(rect, false);
    }

private:
    int doLayout(const QRect& rect, bool testOnly) const {
        int left, top, right, bottom;
        getContentsMargins(&left, &top, &right, &bottom);
        QRect effective = rect.adjusted(left, top, -right, -bottom);
        int x = effective.x();
        int y = effective.y();
        int lineHeight = 0;

        for (QLayoutItem* item : mItems) {
            QSize itemSize = item->sizeHint().expandedTo(item->minimumSize());
            int nextX = x + itemSize.width() + mHSpace;
            if (nextX - mHSpace > effective.right() && lineHeight > 0) {
                x = effective.x();
                y = y + lineHeight + mVSpace;
                nextX = x + itemSize.width() + mHSpace;
                lineHeight = 0;
            }

            if (!testOnly)
                item->setGeometry(QRect(QPoint(x, y), itemSize));

            x = nextX;
            lineHeight = qMax(lineHeight, itemSize.height());
        }
        return y + lineHeight - rect.y() + bottom;
    }

    QList<QLayoutItem*> mItems;
    int mHSpace;
    int mVSpace;
};

#endif // FLOWLAYOUT_H
